#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct SDL_AudioStream;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init(bool headless);
    void shutdown();

    void set_data_dir(const std::string& dir) { data_dir_ = dir; }

    // Music (INT B8h / INT B9h)
    void play_fmu_track(uint8_t file_id);
    void stop_music();

    // Voice / SFX (INT 8Bh)
    void play_voc(int voc_index_1based);

    bool is_playing_music() const { return music_playing_; }

private:
    struct FmuChannel {
        bool active = false;
        uint16_t start_off = 0;
        uint16_t loop_off = 0;
        uint16_t pos = 0;
        double wait_ticks = 0.0;
        double phase = 0.0;
        double mod_phase = 0.0;
        double freq = 0.0;
        double env = 0.0;
        double volume = 0.5;
        bool note_on = false;
    };

    struct VocVoice {
        std::vector<int16_t> samples; // already resampled/converted to signed 16-bit
        uint32_t sample_rate = 8000;
        double pos = 0.0;
        bool active = false;
    };

    std::string data_dir_;
    SDL_AudioStream* stream_ = nullptr;
    bool initialized_ = false;
    mutable std::mutex mutex_;

    // Music state
    std::vector<uint8_t> fmu_data_;
    FmuChannel channels_[10]{};
    double tempo_bpm_ = 120.0;
    double ticks_per_sample_ = 0.0;
    bool music_playing_ = false;

    // Active SFX voices
    std::vector<VocVoice> active_voices_;

    static void sdl_callback(void* userdata, SDL_AudioStream* stream, int additional, int total);
    void mix(int16_t* out, int frames);
    void advance_channel(FmuChannel& ch);
    bool load_voc_file(const std::string& path, VocVoice& out);
};
