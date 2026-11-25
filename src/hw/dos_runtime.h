#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

class Cpu386;
class EgaVga;
class AudioEngine;

class DosRuntime {
public:
    DosRuntime(Cpu386* cpu, EgaVga* vga, AudioEngine* audio);
    ~DosRuntime();

    bool boot(const std::string& data_dir, bool trace_overlays);

    // Input injection from SDL
    void push_key_scancode(uint8_t scancode, uint16_t bios_key, bool pressed);
    void set_mouse_state(int x_640, int y_400, uint16_t buttons);
    void dispatch_mouse_callback();

    // Interrupt and Port callbacks for Cpu386
    bool handle_int(uint8_t vec);
    uint16_t port_in(uint16_t port, int size);
    void port_out(uint16_t port, uint16_t val, int size);

    bool has_pending_scancode() const { return !kbd_scancodes_.empty(); }
    uint8_t pop_scancode_for_irq();

    // Debug: serve file B whenever the game opens file A (key: lower-case DOS name). Set by --remap A=B.
    std::unordered_map<std::string, std::string> remap;

private:
    Cpu386* cpu_ = nullptr;
    EgaVga* vga_ = nullptr;
    AudioEngine* audio_ = nullptr;

    std::string data_dir_;
    bool trace_overlays_ = false;

    // DOS Filesystem & Memory
    struct OpenFile {
        std::FILE* fp = nullptr;
        std::string path;
    };
    std::unordered_map<uint16_t, OpenFile> files_;
    uint16_t next_fd_ = 5;
    uint16_t next_alloc_seg_ = 0x8000;
    uint16_t dta_seg_ = 0x0800;
    uint16_t dta_off_ = 0x0080;
    std::vector<std::string> find_results_;
    size_t find_index_ = 0;

    // EMS 4.0 state (page frame 0xD000, 4 physical pages of 16KB)
    static constexpr uint16_t kEmsPageFrameSeg = 0xD000;
    static constexpr int kEmsTotalPages = 512; // 8 MB
    struct EmsHandle {
        bool active = false;
        std::vector<std::array<uint8_t, 16384>> pages;
    };
    std::unordered_map<uint16_t, EmsHandle> ems_handles_;
    uint16_t next_ems_handle_ = 1;
    int32_t ems_mapped_handle_[4]{-1, -1, -1, -1};
    int32_t ems_mapped_page_[4]{-1, -1, -1, -1};

    // Mouse state
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    uint16_t mouse_buttons_ = 0;
    uint16_t mouse_min_x_ = 0;
    uint16_t mouse_max_x_ = 639;
    uint16_t mouse_min_y_ = 0;
    uint16_t mouse_max_y_ = 399;
    uint16_t mouse_cb_mask_ = 0;
    uint16_t mouse_cb_seg_ = 0;
    uint16_t mouse_cb_off_ = 0;
    bool in_mouse_cb_ = false;
    struct MouseEvent {
        int x;
        int y;
        uint16_t buttons;
        uint16_t ev_mask;
    };
    std::queue<MouseEvent> mouse_events_;

    // Keyboard state
    std::queue<uint8_t> kbd_scancodes_;
    uint8_t last_scancode_ = 0;
    std::queue<uint16_t> bios_keys_;

    // SoundBlaster dummy state
    uint8_t sb_read_byte_ = 0xAA;

    void install_bios_font();
    bool load_mz_exe(const std::string& filename, uint16_t load_seg, uint16_t psp_seg);
    std::string resolve_path(const std::string& dos_path) const;

    bool handle_int10();
    bool handle_int16();
    bool handle_int1a();
    bool handle_int21();
    bool handle_int2f();
    bool handle_int33();
    bool handle_int67();

    void ems_sync_out(int phys_page);
    int ems_free_pages() const;
    std::string read_asciiz(uint16_t seg, uint16_t off) const;
    void write_dta_entry(const std::string& filename);
};
