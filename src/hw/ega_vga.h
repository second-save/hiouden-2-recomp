#pragma once

#include <cstdint>
#include <string>

class EgaVga {
public:
    EgaVga();
    void reset();

    // VRAM access for physical addresses 0xA0000..0xAFFFF (offset = addr - 0xA0000)
    uint8_t vram_read8(uint32_t offset);
    void vram_write8(uint32_t offset, uint8_t val);

    // I/O port access
    uint8_t in8(uint16_t port);
    void out8(uint16_t port, uint8_t val);
    void out16(uint16_t port, uint16_t val);

    // INT 10h palette helpers
    void set_ega_palette(const uint8_t regs[17]);
    void set_dac_block(uint16_t start, uint16_t count, const uint8_t* rgb6);
    void set_dac_reg(uint8_t index, uint8_t r6, uint8_t g6, uint8_t b6);

    // Frame timing & VBlank control
    void begin_frame();
    void enter_vblank();
    bool in_vblank() const { return vblank_active_; }
    uint32_t vblank_poll_count() const { return vblank_poll_count_; }

    // Render 640x400 ARGB8888 output
    void render_argb(uint32_t* out_pixels) const;
    bool save_bmp(const std::string& path) const;

    // Direct diagnostics
    uint16_t display_start() const { return static_cast<uint16_t>((crtc_[0x0C] << 8) | crtc_[0x0D]); }
    uint64_t total_vram_writes() const { return vram_writes_; }

private:
    uint8_t planes_[4][65536]{};
    uint8_t latches_[4]{};

    // Sequencer (0x3C4 / 0x3C5)
    uint8_t seq_index_ = 0;
    uint8_t seq_[8]{}; // seq_[2] = map_mask

    // Graphics Controller (0x3CE / 0x3CF)
    uint8_t gc_index_ = 0;
    uint8_t gc_[16]{};

    // CRTC (0x3D4 / 0x3D5)
    uint8_t crtc_index_ = 0;
    uint8_t crtc_[32]{};

    // Attribute Controller (0x3C0 / 0x3C1)
    bool attr_flip_flop_ = false; // false = index, true = data
    uint8_t attr_index_ = 0;
    uint8_t attr_palette_[16]{};
    bool attr_palette_enabled_ = true;

    // DAC (0x3C7 / 0x3C8 / 0x3C9)
    uint8_t dac_write_idx_ = 0;
    uint8_t dac_write_comp_ = 0;
    uint8_t dac_read_idx_ = 0;
    uint8_t dac_read_comp_ = 0;
    uint8_t dac_[256][3]{};

    // VBlank / Status 0x3DA state
    bool vblank_active_ = false;
    bool vblank_pulse_consumed_ = false;
    uint8_t hblank_toggle_ = 0;
    uint32_t vblank_poll_count_ = 0;
    uint64_t vram_writes_ = 0;

    static uint8_t ror8(uint8_t v, uint8_t count);
    uint8_t apply_alu(uint8_t input, uint8_t latch) const;
};
