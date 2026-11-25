#include "hw/audio.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

constexpr int kSampleRate = 44100;
constexpr double kPi = 3.14159265358979323846;

std::string find_file_ci(const std::string& dir, const std::string& target) {
    fs::path direct = fs::path(dir) / target;
    if (fs::exists(direct)) return direct.string();
    std::string low = target;
    for (char& c : low) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        std::string name = entry.path().filename().string();
        std::string nlow = name;
        for (char& c : nlow) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (nlow == low) return entry.path().string();
    }
    return "";
}

double midi_to_freq(int note) {
    // Wolf Team FMU1 note byte (0x49 around middle A4 / 440Hz, ~12 semitones per octave)
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

} // namespace

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init(bool headless) {
    if (headless) {
        initialized_ = true;
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "[Audio] SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
        return false;
    }
    const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, kSampleRate};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, sdl_callback, this);
    if (!stream_) {
        std::fprintf(stderr, "[Audio] SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_ResumeAudioStreamDevice(stream_);
    initialized_ = true;
    return true;
}

void AudioEngine::shutdown() {
    if (stream_) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
    initialized_ = false;
}

void AudioEngine::play_fmu_track(uint8_t file_id) {
    int fmu_idx = -1;
    if (file_id == 0x50) fmu_idx = 1;
    else if (file_id == 0x51) fmu_idx = 2;
    else if (file_id >= 0x53 && file_id <= 0x67) fmu_idx = (file_id - 0x53) + 3;
    else if (file_id >= 1 && file_id <= 23) fmu_idx = file_id;
    else return;

    char fname[32];
    std::snprintf(fname, sizeof(fname), "FM%02d.FMU", fmu_idx);
    const std::string path = find_file_ci(data_dir_, fname);
    if (path.empty()) return;

    std::ifstream f(path, std::ios::binary);
    if (!f) return;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (data.size() < 0x38 || std::memcmp(data.data(), "FMU1", 4) != 0) return;

    std::lock_guard<std::mutex> lock(mutex_);
    fmu_data_ = std::move(data);
    const uint8_t tempo = fmu_data_[4] ? fmu_data_[4] : 120;
    tempo_bpm_ = static_cast<double>(tempo);
    // 24 ticks per beat
    ticks_per_sample_ = (tempo_bpm_ * 24.0) / (60.0 * static_cast<double>(kSampleRate));

    for (int ch = 0; ch < 10; ++ch) {
        channels_[ch] = FmuChannel{};
        const uint16_t start_off = static_cast<uint16_t>(
            fmu_data_[0x10 + ch * 2] | (fmu_data_[0x10 + ch * 2 + 1] << 8));
        const uint16_t loop_off = static_cast<uint16_t>(
            fmu_data_[0x24 + ch * 2] | (fmu_data_[0x24 + ch * 2 + 1] << 8));
        if (start_off != 0 && start_off < fmu_data_.size() && ch != 8) {
            channels_[ch].active = true;
            channels_[ch].start_off = start_off;
            channels_[ch].loop_off = (loop_off >= start_off && loop_off < fmu_data_.size()) ? loop_off : start_off;
            channels_[ch].pos = start_off;
            channels_[ch].wait_ticks = 0.0;
        }
    }
    music_playing_ = true;
}

void AudioEngine::stop_music() {
    std::lock_guard<std::mutex> lock(mutex_);
    music_playing_ = false;
    for (auto& ch : channels_) ch.active = false;
}

bool AudioEngine::load_voc_file(const std::string& path, VocVoice& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (raw.size() < 32 || std::memcmp(raw.data(), "Creative Voice File", 19) != 0) {
        return false;
    }
    size_t pos = static_cast<size_t>(raw[20] | (raw[21] << 8));
    out.sample_rate = 8000;
    while (pos < raw.size()) {
        const uint8_t btype = raw[pos++];
        if (btype == 0) break;
        if (pos + 3 > raw.size()) break;
        const size_t blen = static_cast<size_t>(raw[pos]) |
                            (static_cast<size_t>(raw[pos + 1]) << 8) |
                            (static_cast<size_t>(raw[pos + 2]) << 16);
        pos += 3;
        if (pos + blen > raw.size()) break;
        if (btype == 1 && blen >= 2) {
            const uint8_t tc = raw[pos];
            out.sample_rate = (tc >= 128) ? (1000000u / (256u - tc)) : 8000u;
            for (size_t i = 2; i < blen; ++i) {
                const int s = (static_cast<int>(raw[pos + i]) - 128) * 200;
                out.samples.push_back(static_cast<int16_t>(std::clamp(s, -32767, 32767)));
            }
        } else if (btype == 2) {
            for (size_t i = 0; i < blen; ++i) {
                const int s = (static_cast<int>(raw[pos + i]) - 128) * 200;
                out.samples.push_back(static_cast<int16_t>(std::clamp(s, -32767, 32767)));
            }
        }
        pos += blen;
    }
    out.pos = 0.0;
    out.active = !out.samples.empty();
    return out.active;
}

void AudioEngine::play_voc(int voc_index_1based) {
    if (voc_index_1based < 1 || voc_index_1based > 15) return;
    char fname[32];
    std::snprintf(fname, sizeof(fname), "B_%02d.VOC", voc_index_1based);
    const std::string path = find_file_ci(data_dir_, fname);
    if (path.empty()) return;

    VocVoice v;
    if (!load_voc_file(path, v)) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (active_voices_.size() > 8) {
        active_voices_.erase(active_voices_.begin());
    }
    active_voices_.push_back(std::move(v));
}

void AudioEngine::advance_channel(FmuChannel& ch) {
    int safety = 0;
    while (ch.active && ch.wait_ticks <= 0.0 && safety++ < 64) {
        if (ch.pos >= fmu_data_.size()) {
            ch.pos = ch.loop_off;
            if (ch.pos >= fmu_data_.size()) {
                ch.active = false;
                return;
            }
        }
        const uint8_t b = fmu_data_[ch.pos++];
        if (b < 0x80) {
            const uint8_t dur = (ch.pos < fmu_data_.size()) ? fmu_data_[ch.pos++] : 0x18;
            ch.freq = midi_to_freq(b);
            ch.note_on = (b > 0);
            ch.env = 1.0;
            ch.wait_ticks += static_cast<double>(dur ? dur : 1);
            break;
        }
        if (b == 0xD2) {
            const uint8_t dur = (ch.pos < fmu_data_.size()) ? fmu_data_[ch.pos++] : 0x18;
            ch.note_on = false;
            ch.wait_ticks += static_cast<double>(dur ? dur : 1);
            break;
        }
        if (b == 0xF0) {
            const uint8_t vol = (ch.pos < fmu_data_.size()) ? fmu_data_[ch.pos++] : 0x40;
            ch.volume = std::clamp(static_cast<double>(vol) / 127.0, 0.05, 0.8);
        } else if (b == 0xE0) {
            ch.pos = static_cast<uint16_t>(std::min<size_t>(ch.pos + 2, fmu_data_.size()));
        } else if (b == 0xF6) {
            ch.pos = static_cast<uint16_t>(std::min<size_t>(ch.pos + 3, fmu_data_.size()));
        } else if (b == 0xFE) {
            // Bar marker
        } else if (b == 0xFF) {
            ch.pos = ch.loop_off;
            if (ch.loop_off == 0 || ch.loop_off >= fmu_data_.size()) {
                ch.active = false;
                return;
            }
        } else {
            if (ch.pos < fmu_data_.size()) ++ch.pos;
        }
    }
}

void AudioEngine::sdl_callback(void* userdata, SDL_AudioStream* stream, int additional, int /*total*/) {
    auto* self = static_cast<AudioEngine*>(userdata);
    const int frames = additional / 4;  // stereo S16
    if (frames <= 0) return;
    std::vector<int16_t> buf(static_cast<size_t>(frames) * 2, 0);
    self->mix(buf.data(), frames);
    SDL_PutAudioStreamData(stream, buf.data(), frames * 4);
}

void AudioEngine::mix(int16_t* out, int frames) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int i = 0; i < frames; ++i) {
        double mix_sample = 0.0;

        if (music_playing_ && !fmu_data_.empty()) {
            for (int c = 0; c < 10; ++c) {
                auto& ch = channels_[c];
                if (!ch.active) continue;
                if (ch.wait_ticks <= 0.0) {
                    advance_channel(ch);
                }
                ch.wait_ticks -= ticks_per_sample_;
                if (ch.note_on && ch.freq > 20.0 && ch.freq < 12000.0) {
                    const double mod = std::sin(2.0 * kPi * ch.mod_phase) * 0.65;
                    const double sig = std::sin(2.0 * kPi * ch.phase + mod);
                    mix_sample += sig * ch.env * ch.volume * 1800.0;
                    ch.phase += ch.freq / static_cast<double>(kSampleRate);
                    ch.mod_phase += (ch.freq * 2.0) / static_cast<double>(kSampleRate);
                    if (ch.phase >= 1.0) ch.phase -= std::floor(ch.phase);
                    if (ch.mod_phase >= 1.0) ch.mod_phase -= std::floor(ch.mod_phase);
                    ch.env *= 0.9997;
                }
            }
        }

        for (auto it = active_voices_.begin(); it != active_voices_.end();) {
            if (!it->active || it->pos >= static_cast<double>(it->samples.size())) {
                it = active_voices_.erase(it);
                continue;
            }
            const size_t idx = static_cast<size_t>(it->pos);
            mix_sample += static_cast<double>(it->samples[idx]) * 0.9;
            it->pos += static_cast<double>(it->sample_rate) / static_cast<double>(kSampleRate);
            ++it;
        }

        const int32_t s = std::clamp(static_cast<int32_t>(mix_sample), -32767, 32767);
        out[i * 2 + 0] = static_cast<int16_t>(s);
        out[i * 2 + 1] = static_cast<int16_t>(s);
    }
}
