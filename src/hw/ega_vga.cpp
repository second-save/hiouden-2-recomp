#include "hw/ega_vga.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>

EgaVga::EgaVga() {
    reset();
}

void EgaVga::reset() {
    std::memset(planes_, 0, sizeof(planes_));
    std::memset(latches_, 0, sizeof(latches_));
    seq_index_ = 0;
    std::memset(seq_, 0, sizeof(seq_));
    seq_[0x02] = 0x0F; // Map Mask = all 4 planes enabled

    gc_index_ = 0;
    std::memset(gc_, 0, sizeof(gc_));
    gc_[0x07] = 0x0F; // Color Don't Care
    gc_[0x08] = 0xFF; // Bit Mask

    crtc_index_ = 0;
    std::memset(crtc_, 0, sizeof(crtc_));

    attr_flip_flop_ = false;
    attr_index_ = 0;
    attr_palette_enabled_ = true;
    for (int i = 0; i < 16; ++i) {
        attr_palette_[i] = static_cast<uint8_t>(i);
    }

    dac_write_idx_ = 0;
    dac_write_comp_ = 0;
    dac_read_idx_ = 0;
    dac_read_comp_ = 0;
    std::memset(dac_, 0, sizeof(dac_));

    // Standard 16-color EGA/VGA default DAC palette (6-bit values 0..63)
    static const uint8_t kDefaultDac[16][3] = {
        {0, 0, 0},    {0, 0, 42},   {0, 42, 0},   {0, 42, 42},
        {42, 0, 0},   {42, 0, 42},  {42, 21, 0},  {42, 42, 42},
        {21, 21, 21}, {21, 21, 63}, {21, 63, 21}, {21, 63, 63},
        {63, 21, 21}, {63, 21, 63}, {63, 63, 21}, {63, 63, 63},
    };
    for (int i = 0; i < 16; ++i) {
        dac_[i][0] = kDefaultDac[i][0];
        dac_[i][1] = kDefaultDac[i][1];
        dac_[i][2] = kDefaultDac[i][2];
    }

    vblank_active_ = false;
    vblank_pulse_consumed_ = false;
    hblank_toggle_ = 0;
    vblank_poll_count_ = 0;
    vram_writes_ = 0;
}

uint8_t EgaVga::ror8(uint8_t v, uint8_t count) {
    count &= 7;
    if (count == 0) return v;
    return static_cast<uint8_t>((v >> count) | (v << (8 - count)));
}

uint8_t EgaVga::apply_alu(uint8_t input, uint8_t latch) const {
    const uint8_t op = (gc_[0x03] >> 3) & 0x03;
    switch (op) {
        case 0: return input;
        case 1: return static_cast<uint8_t>(input & latch);
        case 2: return static_cast<uint8_t>(input | latch);
        case 3: return static_cast<uint8_t>(input ^ latch);
        default: return input;
    }
}

uint8_t EgaVga::vram_read8(uint32_t offset) {
    const uint16_t off = static_cast<uint16_t>(offset & 0xFFFF);
    for (int p = 0; p < 4; ++p) {
        latches_[p] = planes_[p][off];
    }
    const uint8_t read_mode = (gc_[0x05] >> 3) & 0x01;
    if (read_mode == 0) {
        return latches_[gc_[0x04] & 0x03];
    }
    // Read Mode 1: Color Compare
    uint8_t result = 0xFF;
    const uint8_t color_cmp = gc_[0x02] & 0x0F;
    const uint8_t dont_care = gc_[0x07] & 0x0F;
    for (int p = 0; p < 4; ++p) {
        if ((dont_care >> p) & 1) {
            const uint8_t cmp_bits = ((color_cmp >> p) & 1) ? 0xFF : 0x00;
            result &= static_cast<uint8_t>(~(latches_[p] ^ cmp_bits));
        }
    }
    return result;
}

void EgaVga::vram_write8(uint32_t offset, uint8_t val) {
    ++vram_writes_;
    const uint16_t off = static_cast<uint16_t>(offset & 0xFFFF);
    const uint8_t map_mask = seq_[0x02] & 0x0F;
    const uint8_t write_mode = gc_[0x05] & 0x03;
    const uint8_t bit_mask = gc_[0x08];
    const uint8_t rotate = gc_[0x03] & 0x07;
    const uint8_t set_reset = gc_[0x00] & 0x0F;
    const uint8_t enable_sr = gc_[0x01] & 0x0F;

    for (int p = 0; p < 4; ++p) {
        if (((map_mask >> p) & 1) == 0) {
            continue;
        }
        uint8_t out_byte = 0;
        if (write_mode == 0) {
            uint8_t src = 0;
            if ((enable_sr >> p) & 1) {
                src = ((set_reset >> p) & 1) ? 0xFF : 0x00;
            } else {
                src = ror8(val, rotate);
            }
            const uint8_t alu = apply_alu(src, latches_[p]);
            out_byte = static_cast<uint8_t>((alu & bit_mask) | (latches_[p] & ~bit_mask));
        } else if (write_mode == 1) {
            out_byte = latches_[p];
        } else if (write_mode == 2) {
            const uint8_t src = ((val >> p) & 1) ? 0xFF : 0x00;
            const uint8_t alu = apply_alu(src, latches_[p]);
            out_byte = static_cast<uint8_t>((alu & bit_mask) | (latches_[p] & ~bit_mask));
        } else {
            // Write Mode 3
            const uint8_t src = ((set_reset >> p) & 1) ? 0xFF : 0x00;
            const uint8_t eff_mask = static_cast<uint8_t>(ror8(val, rotate) & bit_mask);
            out_byte = static_cast<uint8_t>((src & eff_mask) | (latches_[p] & ~eff_mask));
        }
        planes_[p][off] = out_byte;
    }
}

uint8_t EgaVga::in8(uint16_t port) {
    switch (port) {
        case 0x03C1:
            if ((attr_index_ & 0x1F) < 16) {
                return attr_palette_[attr_index_ & 0x0F];
            }
            return 0;
        case 0x03C4:
            return seq_index_;
        case 0x03C5:
            return (seq_index_ < 8) ? seq_[seq_index_] : 0;
        case 0x03C9: {
            const uint8_t val = dac_[dac_read_idx_][dac_read_comp_];
            dac_read_comp_ = static_cast<uint8_t>((dac_read_comp_ + 1) % 3);
            if (dac_read_comp_ == 0) {
                ++dac_read_idx_;
            }
            return val;
        }
        case 0x03CE:
            return gc_index_;
        case 0x03CF:
            return (gc_index_ < 16) ? gc_[gc_index_] : 0;
        case 0x03D4:
            return crtc_index_;
        case 0x03D5:
            return (crtc_index_ < 32) ? crtc_[crtc_index_] : 0;
        case 0x03DA: {
            attr_flip_flop_ = false;
            ++vblank_poll_count_;
            hblank_toggle_ ^= 0x01;
            uint8_t status = hblank_toggle_;
            if (vblank_active_ && !vblank_pulse_consumed_) {
                status |= 0x09; // VBlank (bit 3) + Display disabled (bit 0)
                vblank_pulse_consumed_ = true;
                vblank_active_ = false;
            }
            return status;
        }
        default:
            return 0xFF;
    }
}

void EgaVga::out8(uint16_t port, uint8_t val) {
    switch (port) {
        case 0x03C0:
            if (!attr_flip_flop_) {
                attr_index_ = val & 0x1F;
                attr_palette_enabled_ = (val & 0x20) != 0;
            } else {
                if (attr_index_ < 16) {
                    attr_palette_[attr_index_] = val & 0x3F;
                }
            }
            attr_flip_flop_ = !attr_flip_flop_;
            break;
        case 0x03C4:
            seq_index_ = val & 0x07;
            break;
        case 0x03C5:
            if (seq_index_ < 8) {
                seq_[seq_index_] = val;
            }
            break;
        case 0x03C7:
            dac_read_idx_ = val;
            dac_read_comp_ = 0;
            break;
        case 0x03C8:
            dac_write_idx_ = val;
            dac_write_comp_ = 0;
            break;
        case 0x03C9:
            dac_[dac_write_idx_][dac_write_comp_] = val & 0x3F;
            dac_write_comp_ = static_cast<uint8_t>((dac_write_comp_ + 1) % 3);
            if (dac_write_comp_ == 0) {
                ++dac_write_idx_;
            }
            break;
        case 0x03CE:
            gc_index_ = val & 0x0F;
            break;
        case 0x03CF:
            if (gc_index_ < 16) {
                gc_[gc_index_] = val;
            }
            break;
        case 0x03D4:
            crtc_index_ = val & 0x1F;
            break;
        case 0x03D5:
            if (crtc_index_ < 32) {
                crtc_[crtc_index_] = val;
            }
            break;
        default:
            break;
    }
}

void EgaVga::out16(uint16_t port, uint16_t val) {
    out8(port, static_cast<uint8_t>(val & 0xFF));
    out8(static_cast<uint16_t>(port + 1), static_cast<uint8_t>((val >> 8) & 0xFF));
}

void EgaVga::set_ega_palette(const uint8_t regs[17]) {
    for (int i = 0; i < 16; ++i) {
        attr_palette_[i] = regs[i] & 0x3F;
    }
}

void EgaVga::set_dac_block(uint16_t start, uint16_t count, const uint8_t* rgb6) {
    for (uint16_t i = 0; i < count; ++i) {
        const uint8_t idx = static_cast<uint8_t>((start + i) & 0xFF);
        dac_[idx][0] = rgb6[i * 3 + 0] & 0x3F;
        dac_[idx][1] = rgb6[i * 3 + 1] & 0x3F;
        dac_[idx][2] = rgb6[i * 3 + 2] & 0x3F;
    }
}

void EgaVga::set_dac_reg(uint8_t index, uint8_t r6, uint8_t g6, uint8_t b6) {
    dac_[index][0] = r6 & 0x3F;
    dac_[index][1] = g6 & 0x3F;
    dac_[index][2] = b6 & 0x3F;
}

void EgaVga::begin_frame() {
    vblank_active_ = false;
    vblank_pulse_consumed_ = false;
}

void EgaVga::enter_vblank() {
    if (!vblank_pulse_consumed_) {
        vblank_active_ = true;
    }
}

void EgaVga::render_argb(uint32_t* out_pixels) const {
    const uint16_t start = display_start();
    uint32_t pal32[16];
    for (int c = 0; c < 16; ++c) {
        const uint8_t dac_idx = attr_palette_[c];
        const uint32_t r = static_cast<uint32_t>(dac_[dac_idx][0] * 255 / 63);
        const uint32_t g = static_cast<uint32_t>(dac_[dac_idx][1] * 255 / 63);
        const uint32_t b = static_cast<uint32_t>(dac_[dac_idx][2] * 255 / 63);
        pal32[c] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }

    for (int y = 0; y < 400; ++y) {
        const uint16_t row_off = static_cast<uint16_t>(start + y * 80);
        uint32_t* dst = out_pixels + y * 640;
        for (int xb = 0; xb < 80; ++xb) {
            const uint16_t off = static_cast<uint16_t>(row_off + xb);
            const uint8_t p0 = planes_[0][off];
            const uint8_t p1 = planes_[1][off];
            const uint8_t p2 = planes_[2][off];
            const uint8_t p3 = planes_[3][off];
            for (int bit = 7; bit >= 0; --bit) {
                const uint8_t idx = static_cast<uint8_t>(
                    (((p0 >> bit) & 1) << 0) |
                    (((p1 >> bit) & 1) << 1) |
                    (((p2 >> bit) & 1) << 2) |
                    (((p3 >> bit) & 1) << 3)
                );
                *dst++ = pal32[idx];
            }
        }
    }
}

bool EgaVga::save_bmp(const std::string& path) const {
    std::vector<uint32_t> argb(640 * 400);
    render_argb(argb.data());

    constexpr int W = 640;
    constexpr int H = 400;
    constexpr uint32_t row_bytes = W * 3;
    constexpr uint32_t image_bytes = row_bytes * H;
    constexpr uint32_t file_size = 54 + image_bytes;

    uint8_t hdr[54]{};
    hdr[0] = 'B';
    hdr[1] = 'M';
    std::memcpy(hdr + 2, &file_size, 4);
    uint32_t offset = 54;
    std::memcpy(hdr + 10, &offset, 4);
    uint32_t dib_size = 40;
    std::memcpy(hdr + 14, &dib_size, 4);
    int32_t w = W;
    int32_t h = H;
    std::memcpy(hdr + 18, &w, 4);
    std::memcpy(hdr + 22, &h, 4);
    uint16_t planes = 1;
    uint16_t bpp = 24;
    std::memcpy(hdr + 26, &planes, 2);
    std::memcpy(hdr + 28, &bpp, 2);
    std::memcpy(hdr + 34, &image_bytes, 4);

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(hdr), 54);

    std::vector<uint8_t> row(row_bytes);
    for (int y = H - 1; y >= 0; --y) {
        const uint32_t* src = argb.data() + y * W;
        for (int x = 0; x < W; ++x) {
            const uint32_t px = src[x];
            row[x * 3 + 0] = static_cast<uint8_t>(px & 0xFF);         // B
            row[x * 3 + 1] = static_cast<uint8_t>((px >> 8) & 0xFF);  // G
            row[x * 3 + 2] = static_cast<uint8_t>((px >> 16) & 0xFF); // R
        }
        f.write(reinterpret_cast<const char*>(row.data()), row_bytes);
    }
    return f.good();
}
