#include "hw/dos_runtime.h"
#include "cpu/cpu386.h"
#include "hw/audio.h"
#include "hw/ega_vga.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

// Compact standard 8x8 IBM BIOS font for ASCII 0x20..0x7E (95 glyphs * 8 bytes)
const uint8_t kBiosFont8x8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // '!'
    {0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00}, // '"'
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // '#'
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // '$'
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, // '%'
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // '&'
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // '\''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // '.'
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // '/'
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // '1'
    {0x7C,0xC6,0x0C,0x18,0x30,0x60,0xFE,0x00}, // '2'
    {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00}, // '3'
    {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00}, // '4'
    {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, // '5'
    {0x38,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, // '6'
    {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00}, // '7'
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, // '8'
    {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00}, // '9'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // ':'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // '<'
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // '='
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // '>'
    {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00}, // '?'
    {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x78,0x00}, // '@'
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 'A'
    {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, // 'B'
    {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, // 'C'
    {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, // 'D'
    {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, // 'E'
    {0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00}, // 'F'
    {0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00}, // 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 'I'
    {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00}, // 'J'
    {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00}, // 'K'
    {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, // 'L'
    {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00}, // 'M'
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, // 'N'
    {0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // 'O'
    {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, // 'P'
    {0x7C,0xC6,0xC6,0xC6,0xD6,0x7C,0x0E,0x00}, // 'Q'
    {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, // 'R'
    {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00}, // 'S'
    {0x7E,0x7E,0x5A,0x18,0x18,0x18,0x3C,0x00}, // 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 'V'
    {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00}, // 'W'
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, // 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00}, // 'Y'
    {0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00}, // 'Z'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // '['
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // '\\'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // ']'
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, // '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // '_'
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // '`'
    {0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, // 'a'
    {0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0x00}, // 'b'
    {0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00}, // 'c'
    {0x1C,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00}, // 'd'
    {0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00}, // 'e'
    {0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00}, // 'f'
    {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8}, // 'g'
    {0xE0,0x60,0x7C,0x66,0x66,0x66,0xE6,0x00}, // 'h'
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 'i'
    {0x0C,0x00,0x1C,0x0C,0x0C,0xCC,0xCC,0x78}, // 'j'
    {0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00}, // 'k'
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 'l'
    {0x00,0x00,0xCC,0xFE,0xFE,0xD6,0xC6,0x00}, // 'm'
    {0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00}, // 'n'
    {0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00}, // 'o'
    {0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0}, // 'p'
    {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E}, // 'q'
    {0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00}, // 'r'
    {0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00}, // 's'
    {0x10,0x30,0xFC,0x30,0x30,0x34,0x18,0x00}, // 't'
    {0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00}, // 'u'
    {0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00}, // 'v'
    {0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00}, // 'w'
    {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, // 'x'
    {0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8}, // 'y'
    {0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00}, // 'z'
    {0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00}, // '{'
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // '|'
    {0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00}, // '}'
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}, // '~'
};

std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool wildcard_match(const std::string& pat, const std::string& name) {
    size_t pi = 0, ni = 0, star_p = std::string::npos, star_n = 0;
    while (ni < name.size()) {
        if (pi < pat.size() && (pat[pi] == '?' || pat[pi] == name[ni])) {
            ++pi;
            ++ni;
        } else if (pi < pat.size() && pat[pi] == '*') {
            star_p = pi++;
            star_n = ni;
        } else if (star_p != std::string::npos) {
            pi = star_p + 1;
            ni = ++star_n;
        } else {
            return false;
        }
    }
    while (pi < pat.size() && pat[pi] == '*') ++pi;
    return pi == pat.size();
}

} // namespace

DosRuntime::DosRuntime(Cpu386* cpu, EgaVga* vga, AudioEngine* audio)
    : cpu_(cpu), vga_(vga), audio_(audio) {}

DosRuntime::~DosRuntime() {
    for (auto& kv : files_) {
        if (kv.second.fp) std::fclose(kv.second.fp);
    }
    files_.clear();
}

void DosRuntime::install_bios_font() {
    const uint32_t base = Cpu386::phys(0xF000, 0xFA6E);
    std::memset(cpu_->ram() + base, 0, 128 * 8);
    for (int ch = 0x20; ch <= 0x7E; ++ch) {
        std::memcpy(cpu_->ram() + base + ch * 8, kBiosFont8x8[ch - 0x20], 8);
    }
}

std::string DosRuntime::resolve_path(const std::string& dos_path) const {
    std::string p = dos_path;
    // Strip drive prefix like J:, A:, C:, F:
    if (p.size() >= 2 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':') {
        p = p.substr(2);
    }
    for (char& c : p) {
        if (c == '\\') c = '/';
    }
    while (!p.empty() && p[0] == '/') {
        p = p.substr(1);
    }
    if (to_lower(p).rfind("game/", 0) == 0) {
        p = p.substr(5);
    }
    // Trim whitespace
    while (!p.empty() && std::isspace(static_cast<unsigned char>(p.back()))) {
        p.pop_back();
    }
    const auto rm = remap.find(to_lower(p));
    if (rm != remap.end()) p = rm->second;

    fs::path direct = fs::path(data_dir_) / p;
    if (fs::exists(direct)) return direct.string();

    // Case-insensitive lookup inside data_dir_
    const std::string target_low = to_lower(p);
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(data_dir_, ec)) {
        if (to_lower(entry.path().filename().string()) == target_low) {
            return entry.path().string();
        }
    }
    return direct.string();
}

bool DosRuntime::load_mz_exe(const std::string& filename, uint16_t load_seg, uint16_t psp_seg) {
    const std::string path = resolve_path(filename);
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "[DOS] Failed to open %s\n", path.c_str());
        return false;
    }
    std::vector<uint8_t> exe((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (exe.size() < 28 || exe[0] != 'M' || exe[1] != 'Z') {
        std::fprintf(stderr, "[DOS] Invalid MZ executable: %s\n", path.c_str());
        return false;
    }

    auto u16 = [&](size_t off) -> uint16_t {
        return static_cast<uint16_t>(exe[off] | (exe[off + 1] << 8));
    };

    const uint16_t reloc_cnt = u16(0x06);
    const uint16_t hdr_paras = u16(0x08);
    const uint16_t ss_init = u16(0x0E);
    const uint16_t sp_init = u16(0x10);
    const uint16_t ip_init = u16(0x14);
    const uint16_t cs_init = u16(0x16);
    const uint16_t reloc_off = u16(0x18);

    const size_t img_off = static_cast<size_t>(hdr_paras) * 16;
    if (img_off >= exe.size()) return false;
    const size_t img_size = exe.size() - img_off;

    // Build minimal PSP
    const uint32_t psp_phys = Cpu386::phys(psp_seg, 0);
    std::memset(cpu_->ram() + psp_phys, 0, 256);
    cpu_->write8(psp_phys + 0x00, 0xCD); // INT 20h
    cpu_->write8(psp_phys + 0x01, 0x20);
    cpu_->write16(psp_phys + 0x02, 0x9FFF); // top of conventional memory seg
    cpu_->write16(psp_phys + 0x2C, 0x07F0); // dummy environment segment

    const uint32_t load_phys = Cpu386::phys(load_seg, 0);
    std::memcpy(cpu_->ram() + load_phys, exe.data() + img_off, img_size);

    for (uint16_t i = 0; i < reloc_cnt; ++i) {
        const size_t r = reloc_off + static_cast<size_t>(i) * 4;
        if (r + 4 > exe.size()) break;
        const uint16_t off = u16(r);
        const uint16_t seg = u16(r + 2);
        const uint32_t p = Cpu386::phys(static_cast<uint16_t>(load_seg + seg), off);
        cpu_->write16(p, static_cast<uint16_t>(cpu_->read16(p) + load_seg));
    }

    cpu_->sreg[Cpu386::SEG_CS] = static_cast<uint16_t>(load_seg + cs_init);
    cpu_->ip = ip_init;
    cpu_->sreg[Cpu386::SEG_SS] = (ss_init == 0 && sp_init == 0) ? load_seg : static_cast<uint16_t>(load_seg + ss_init);
    cpu_->set_sp((sp_init == 0) ? 0xFFFE : sp_init);
    cpu_->sreg[Cpu386::SEG_DS] = psp_seg;
    cpu_->sreg[Cpu386::SEG_ES] = psp_seg;
    cpu_->sreg[Cpu386::SEG_FS] = 0;
    cpu_->sreg[Cpu386::SEG_GS] = 0;
    cpu_->eflags = 0x0202;

    if (trace_overlays_) {
        std::fprintf(stderr, "[DOS] Loaded %s at %04X:0000 (%zu bytes, %u relocs), entry=%04X:%04X SS:SP=%04X:%04X\n",
                     filename.c_str(), load_seg, img_size, reloc_cnt,
                     cpu_->sreg[Cpu386::SEG_CS], cpu_->ip,
                     cpu_->sreg[Cpu386::SEG_SS], cpu_->sp());
    }
    return true;
}

bool DosRuntime::boot(const std::string& data_dir, bool trace_overlays) {
    data_dir_ = data_dir;
    trace_overlays_ = trace_overlays;
    audio_->set_data_dir(data_dir_);

    cpu_->reset();
    vga_->reset();

    // Populate IVT 0..255 -> F000:0000..00FF (each byte is IRET 0xCF)
    for (int vec = 0; vec < 256; ++vec) {
        cpu_->write8(Cpu386::phys(0xF000, vec), 0xCF);
        cpu_->write16(static_cast<uint32_t>(vec * 4 + 0), static_cast<uint16_t>(vec));
        cpu_->write16(static_cast<uint32_t>(vec * 4 + 2), 0xF000);
    }

    // EMS signature at E000:000A and INT 67h vector -> E000:0000
    cpu_->write8(Cpu386::phys(0xE000, 0x0000), 0xCF);
    const char* emm_sig = "EMMXXXX0";
    std::memcpy(cpu_->ram() + Cpu386::phys(0xE000, 0x000A), emm_sig, 8);
    cpu_->write16(0x67 * 4 + 0, 0x0000);
    cpu_->write16(0x67 * 4 + 2, 0xE000);

    // Initialize BDA (0040:0000..00FF)
    cpu_->write16(0x0410, 0x0021); // Equipment word
    cpu_->write16(0x0413, 640);    // Conventional memory KB
    cpu_->write16(0x041A, 0x001E); // Kbd buf head
    cpu_->write16(0x041C, 0x001E); // Kbd buf tail
    cpu_->write16(0x044A, 80);     // Screen columns
    cpu_->write8(0x0449, 0x12);    // Video mode
    cpu_->write16(0x0450, 0x0000); // HIOSYS timer counter
    cpu_->write16(0x0480, 0x001E);
    cpu_->write16(0x0482, 0x003E);

    mouse_x_ = 0;
    mouse_y_ = 0;
    mouse_buttons_ = 0;
    mouse_cb_mask_ = 0;
    mouse_cb_seg_ = 0;
    mouse_cb_off_ = 0;
    in_mouse_cb_ = false;
    while (!mouse_events_.empty()) mouse_events_.pop();

    install_bios_font();

    return load_mz_exe("HIOSYS.EXE", 0x0810, 0x0800);
}

void DosRuntime::push_key_scancode(uint8_t scancode, uint16_t bios_key, bool pressed) {
    const uint8_t code = pressed ? (scancode & 0x7Fu) : (scancode | 0x80u);
    kbd_scancodes_.push(code);
    if (pressed && bios_key != 0) {
        bios_keys_.push(bios_key);
    }
}

uint8_t DosRuntime::pop_scancode_for_irq() {
    if (!kbd_scancodes_.empty()) {
        last_scancode_ = kbd_scancodes_.front();
        kbd_scancodes_.pop();
    }
    return last_scancode_;
}

void DosRuntime::set_mouse_state(int x_640, int y_400, uint16_t buttons) {
    const int nx = std::clamp(x_640, 0, 639);
    const int ny = std::clamp(y_400, 0, 399);
    uint16_t ev_mask = 0;
    if (nx != mouse_x_ || ny != mouse_y_) ev_mask |= 0x01;
    if ((buttons & 1) && !(mouse_buttons_ & 1)) ev_mask |= 0x02;
    if (!(buttons & 1) && (mouse_buttons_ & 1)) ev_mask |= 0x04;
    if ((buttons & 2) && !(mouse_buttons_ & 2)) ev_mask |= 0x08;
    if (!(buttons & 2) && (mouse_buttons_ & 2)) ev_mask |= 0x10;

    mouse_x_ = nx;
    mouse_y_ = ny;
    mouse_buttons_ = buttons;

    if (ev_mask != 0) {
        if ((ev_mask & 0x1E) == 0 && !mouse_events_.empty() &&
            (mouse_events_.back().ev_mask & 0x1E) == 0) {
            mouse_events_.back().x = nx;
            mouse_events_.back().y = ny;
            mouse_events_.back().buttons = buttons;
            mouse_events_.back().ev_mask |= 0x01;
        } else {
            mouse_events_.push(MouseEvent{nx, ny, buttons, ev_mask});
        }
    }
}

void DosRuntime::dispatch_mouse_callback() {
    if (in_mouse_cb_ || mouse_events_.empty()) return;
    if (mouse_cb_mask_ == 0 || mouse_cb_seg_ == 0) {
        while (!mouse_events_.empty()) mouse_events_.pop();
        return;
    }
    if (!cpu_->get_flag(Cpu386::FLAG_IF)) return;

    MouseEvent ev = mouse_events_.front();
    mouse_events_.pop();
    // Collapse consecutive pure-motion events while stopping at any button event
    while ((ev.ev_mask & 0x1E) == 0 && !mouse_events_.empty() &&
           (mouse_events_.front().ev_mask & 0x1E) == 0) {
        ev.x = mouse_events_.front().x;
        ev.y = mouse_events_.front().y;
        ev.buttons = mouse_events_.front().buttons;
        ev.ev_mask |= mouse_events_.front().ev_mask;
        mouse_events_.pop();
    }

    const uint16_t active = ev.ev_mask & mouse_cb_mask_;
    if (active == 0) return;

    const int rx = (mouse_max_x_ > mouse_min_x_)
        ? (mouse_min_x_ + (ev.x * (mouse_max_x_ - mouse_min_x_)) / 639)
        : ev.x;
    const int ry = (mouse_max_y_ > mouse_min_y_)
        ? (mouse_min_y_ + (ev.y * (mouse_max_y_ - mouse_min_y_)) / 399)
        : ev.y;

    // Save CPU state
    const uint32_t saved_eax = cpu_->eax;
    const uint32_t saved_ecx = cpu_->ecx;
    const uint32_t saved_edx = cpu_->edx;
    const uint32_t saved_ebx = cpu_->ebx;
    const uint32_t saved_esp = cpu_->esp;
    const uint32_t saved_ebp = cpu_->ebp;
    const uint32_t saved_esi = cpu_->esi;
    const uint32_t saved_edi = cpu_->edi;
    uint16_t saved_sreg[6];
    std::memcpy(saved_sreg, cpu_->sreg, sizeof(saved_sreg));
    const uint16_t saved_ip = cpu_->ip;
    const uint32_t saved_eflags = cpu_->eflags;

    // Push sentinel return address 0xF000:0x00FE
    cpu_->push16(0xF000);
    cpu_->push16(0x00FE);

    cpu_->set_ax(active);
    cpu_->set_bx(ev.buttons);
    cpu_->set_cx(static_cast<uint16_t>(rx));
    cpu_->set_dx(static_cast<uint16_t>(ry));
    cpu_->set_si(0);
    cpu_->set_di(0);
    cpu_->sreg[Cpu386::SEG_CS] = mouse_cb_seg_;
    cpu_->ip = mouse_cb_off_;

    in_mouse_cb_ = true;
    for (int step = 0; step < 50000 && !cpu_->halted(); ++step) {
        if (cpu_->sreg[Cpu386::SEG_CS] == 0xF000 && cpu_->ip == 0x00FE) {
            break;
        }
        if (!cpu_->step()) break;
    }
    in_mouse_cb_ = false;

    // Restore CPU state
    cpu_->eax = saved_eax;
    cpu_->ecx = saved_ecx;
    cpu_->edx = saved_edx;
    cpu_->ebx = saved_ebx;
    cpu_->esp = saved_esp;
    cpu_->ebp = saved_ebp;
    cpu_->esi = saved_esi;
    cpu_->edi = saved_edi;
    std::memcpy(cpu_->sreg, saved_sreg, sizeof(saved_sreg));
    cpu_->ip = saved_ip;
    cpu_->eflags = saved_eflags;
}

std::string DosRuntime::read_asciiz(uint16_t seg, uint16_t off) const {
    std::string out;
    for (uint16_t i = 0; i < 260; ++i) {
        const uint8_t ch = cpu_->seg_read8(seg, static_cast<uint16_t>(off + i));
        if (ch == 0) break;
        out.push_back(static_cast<char>(ch));
    }
    return out;
}

void DosRuntime::write_dta_entry(const std::string& filename) {
    const uint32_t dta = Cpu386::phys(dta_seg_, dta_off_);
    std::memset(cpu_->ram() + dta, 0, 43);
    cpu_->write8(dta + 0x15, 0x20); // archive attr
    const fs::path p = fs::path(data_dir_) / filename;
    std::error_code ec;
    const uint32_t sz = fs::exists(p, ec) ? static_cast<uint32_t>(fs::file_size(p, ec)) : 0u;
    cpu_->write32(dta + 0x1A, sz);
    std::string upper = filename;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (size_t i = 0; i < std::min<size_t>(upper.size(), 12); ++i) {
        cpu_->write8(dta + 0x1E + static_cast<uint32_t>(i), static_cast<uint8_t>(upper[i]));
    }
}

bool DosRuntime::handle_int(uint8_t vec) {
    switch (vec) {
        case 0x08:
        case 0x09:
            // Let guest-installed ISR run if hooked; if pointing to F000 IRET, handle timer/kbd
            if (cpu_->read16(vec * 4 + 2) == 0xF000) {
                if (vec == 0x08) {
                    cpu_->write16(0x0450, static_cast<uint16_t>(cpu_->read16(0x0450) + 1));
                } else {
                    pop_scancode_for_irq();
                }
                return true;
            }
            return false;

        case 0x10: return handle_int10();
        case 0x16: return handle_int16();
        case 0x1A: return handle_int1a();
        case 0x20:
            cpu_->set_halted(true);
            return true;
        case 0x21: return handle_int21();
        case 0x2F: return handle_int2f();
        case 0x33: return handle_int33();
        case 0x67: return handle_int67();

        case 0x8B: { // Voice SFX playback
            const int id = cpu_->ax();
            if (trace_overlays_) {
                std::fprintf(stderr, "[AUDIO] INT 8Bh play VOC AX=%d (0x%04X)\n", id, id);
            }
            if (id >= 0 && id < 15) {
                audio_->play_voc(id + 1);
            } else {
                audio_->play_voc(std::clamp(id, 1, 15));
            }
            return true;
        }

        case 0x8E: // Overlay / sub-program loader (HIOSYS.EXE)
            if (trace_overlays_) {
                std::fprintf(stderr, "[OVERLAY] INT 8Eh AX=0x%04X BX=0x%04X CX=0x%04X DX=0x%04X\n",
                             cpu_->ax(), cpu_->bx(), cpu_->cx(), cpu_->dx());
            }
            return false; // Run HIOSYS.EXE's own INT 8Eh handler

        case 0xB5: // File-ID read/write (HIOSYS.EXE)
            if (trace_overlays_) {
                std::fprintf(stderr, "[FILEID] INT B5h AX=0x%04X DX=0x%04X BX:CX=%04X:%04X\n",
                             cpu_->ax(), cpu_->dx(), cpu_->bx(), cpu_->cx());
            }
            return false; // Run HIOSYS.EXE's own INT B5h handler

        case 0xB8:
        case 0xB9: { // Music driver
            const uint8_t fn = cpu_->ah();
            const uint8_t track = cpu_->al();
            if (trace_overlays_) {
                std::fprintf(stderr, "[AUDIO] INT %02Xh AH=0x%02X AL=0x%02X\n", vec, fn, track);
            }
            if (fn == 0x05) {
                audio_->play_fmu_track(track);
            } else if (fn == 0x06 || fn == 0x07) {
                audio_->stop_music();
            }
            cpu_->set_ax(0);
            return true;
        }

        default:
            return false;
    }
}

bool DosRuntime::handle_int10() {
    const uint8_t ah = cpu_->ah();
    if (ah == 0x00) {
        cpu_->write8(0x0449, cpu_->al());
        return true;
    }
    if (ah == 0x0F) {
        const uint8_t mode = cpu_->read8(0x0449);
        cpu_->set_ax(static_cast<uint16_t>((80u << 8) | mode));
        cpu_->set_bx(cpu_->bx() & 0x00FFu); // BH = 0 (page 0)
        return true;
    }
    if (cpu_->ax() == 0x1000) {
        return true;
    }
    if (cpu_->ax() == 0x1002) {
        uint8_t regs[17];
        for (int i = 0; i < 17; ++i) {
            regs[i] = cpu_->seg_read8(cpu_->sreg[Cpu386::SEG_ES], static_cast<uint16_t>(cpu_->dx() + i));
        }
        vga_->set_ega_palette(regs);
        return true;
    }
    if (cpu_->ax() == 0x1010) {
        vga_->set_dac_reg(cpu_->bl(), cpu_->dh(), cpu_->ch(), cpu_->cl());
        return true;
    }
    if (cpu_->ax() == 0x1012) {
        const uint16_t start = cpu_->bx();
        const uint16_t count = cpu_->cx();
        std::vector<uint8_t> rgb(count * 3);
        for (uint16_t i = 0; i < count * 3; ++i) {
            rgb[i] = cpu_->seg_read8(cpu_->sreg[Cpu386::SEG_ES], static_cast<uint16_t>(cpu_->dx() + i));
        }
        vga_->set_dac_block(start, count, rgb.data());
        return true;
    }
    if (cpu_->ax() == 0x1130) {
        cpu_->sreg[Cpu386::SEG_ES] = 0xF000;
        cpu_->set_bp(0xFA6E);
        cpu_->set_cx(8);
        cpu_->set_dx(24);
        return true;
    }
    return true;
}

bool DosRuntime::handle_int16() {
    const uint8_t ah = cpu_->ah();
    if (ah == 0x01 || ah == 0x11) {
        if (bios_keys_.empty()) {
            cpu_->set_flag(Cpu386::FLAG_ZF, true);
        } else {
            cpu_->set_flag(Cpu386::FLAG_ZF, false);
            cpu_->set_ax(bios_keys_.front());
        }
        return true;
    }
    if (ah == 0x00 || ah == 0x10) {
        if (!bios_keys_.empty()) {
            cpu_->set_ax(bios_keys_.front());
            bios_keys_.pop();
        } else {
            cpu_->set_ax(0);
        }
        return true;
    }
    if (ah == 0x02 || ah == 0x12) {
        cpu_->set_al(0);
        return true;
    }
    return true;
}

bool DosRuntime::handle_int1a() {
    if (cpu_->ah() == 0x00) {
        const uint32_t ticks = cpu_->read16(0x0450);
        cpu_->set_cx(static_cast<uint16_t>(ticks >> 16));
        cpu_->set_dx(static_cast<uint16_t>(ticks & 0xFFFF));
        cpu_->set_al(0);
    }
    return true;
}

bool DosRuntime::handle_int21() {
    const uint8_t ah = cpu_->ah();
    switch (ah) {
        case 0x09: { // Print $-terminated string
            std::string msg;
            for (uint16_t i = 0; i < 512; ++i) {
                const uint8_t c = cpu_->seg_read8(cpu_->sreg[Cpu386::SEG_DS], static_cast<uint16_t>(cpu_->dx() + i));
                if (c == '$') break;
                msg.push_back(static_cast<char>(c));
            }
            std::fprintf(stderr, "[DOS stdout] %s\n", msg.c_str());
            return true;
        }
        case 0x19: // Current drive
            cpu_->set_al(2); // C:
            return true;
        case 0x1A: // Set DTA
            dta_seg_ = cpu_->sreg[Cpu386::SEG_DS];
            dta_off_ = cpu_->dx();
            return true;
        case 0x25: { // Set IVT
            const uint8_t vec = cpu_->al();
            cpu_->write16(vec * 4 + 0, cpu_->dx());
            cpu_->write16(vec * 4 + 2, cpu_->sreg[Cpu386::SEG_DS]);
            return true;
        }
        case 0x30: // DOS version
            cpu_->set_ax(0x0005);
            cpu_->set_bx(0);
            cpu_->set_cx(0);
            return true;
        case 0x35: { // Get IVT
            const uint8_t vec = cpu_->al();
            cpu_->set_bx(cpu_->read16(vec * 4 + 0));
            cpu_->sreg[Cpu386::SEG_ES] = cpu_->read16(vec * 4 + 2);
            return true;
        }
        case 0x3C: { // Create / truncate file
            const std::string dos_p = read_asciiz(cpu_->sreg[Cpu386::SEG_DS], cpu_->dx());
            const std::string host_p = resolve_path(dos_p);
            if (trace_overlays_) std::fprintf(stderr, "[DOS] CREATE %s -> %s\n", dos_p.c_str(), host_p.c_str());
            std::FILE* fp = std::fopen(host_p.c_str(), "w+b");
            if (!fp) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(5);
                return true;
            }
            const uint16_t fd = next_fd_++;
            files_[fd] = OpenFile{fp, host_p};
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            cpu_->set_ax(fd);
            return true;
        }
        case 0x3D: { // Open file
            const std::string dos_p = read_asciiz(cpu_->sreg[Cpu386::SEG_DS], cpu_->dx());
            const std::string host_p = resolve_path(dos_p);
            const uint8_t mode = cpu_->al() & 0x03;
            const char* fmode = (mode == 0) ? "rb" : "r+b";
            std::FILE* fp = std::fopen(host_p.c_str(), fmode);
            if (!fp && mode != 0) fp = std::fopen(host_p.c_str(), "rb");
            if (trace_overlays_) {
                std::fprintf(stderr, "[DOS] OPEN(%d) %s -> %s (%s)\n",
                             mode, dos_p.c_str(), host_p.c_str(), fp ? "OK" : "FAIL");
            }
            if (!fp) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(2);
                return true;
            }
            const uint16_t fd = next_fd_++;
            files_[fd] = OpenFile{fp, host_p};
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            cpu_->set_ax(fd);
            return true;
        }
        case 0x3E: { // Close file
            const uint16_t fd = cpu_->bx();
            auto it = files_.find(fd);
            if (it != files_.end()) {
                if (it->second.fp) std::fclose(it->second.fp);
                files_.erase(it);
                cpu_->set_flag(Cpu386::FLAG_CF, false);
            } else {
                cpu_->set_flag(Cpu386::FLAG_CF, false);
            }
            return true;
        }
        case 0x3F: { // Read file
            const uint16_t fd = cpu_->bx();
            const uint16_t cnt = cpu_->cx();
            auto it = files_.find(fd);
            if (it == files_.end() || !it->second.fp) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(6);
                return true;
            }
            std::vector<uint8_t> buf(cnt);
            const size_t n = std::fread(buf.data(), 1, cnt, it->second.fp);
            const uint16_t seg = cpu_->sreg[Cpu386::SEG_DS];
            const uint16_t off = cpu_->dx();
            for (size_t i = 0; i < n; ++i) {
                cpu_->seg_write8(seg, static_cast<uint16_t>(off + i), buf[i]);
            }
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            cpu_->set_ax(static_cast<uint16_t>(n));
            return true;
        }
        case 0x40: { // Write file
            const uint16_t fd = cpu_->bx();
            const uint16_t cnt = cpu_->cx();
            const uint16_t seg = cpu_->sreg[Cpu386::SEG_DS];
            const uint16_t off = cpu_->dx();
            std::vector<uint8_t> buf(cnt);
            for (uint16_t i = 0; i < cnt; ++i) {
                buf[i] = cpu_->seg_read8(seg, static_cast<uint16_t>(off + i));
            }
            if (fd == 1 || fd == 2) {
                std::fwrite(buf.data(), 1, cnt, stderr);
                cpu_->set_flag(Cpu386::FLAG_CF, false);
                cpu_->set_ax(cnt);
                return true;
            }
            auto it = files_.find(fd);
            if (it == files_.end() || !it->second.fp) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(6);
                return true;
            }
            const size_t n = std::fwrite(buf.data(), 1, cnt, it->second.fp);
            std::fflush(it->second.fp);
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            cpu_->set_ax(static_cast<uint16_t>(n));
            return true;
        }
        case 0x42: { // Lseek
            const uint16_t fd = cpu_->bx();
            auto it = files_.find(fd);
            if (it == files_.end() || !it->second.fp) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(6);
                return true;
            }
            const int32_t offset = static_cast<int32_t>((static_cast<uint32_t>(cpu_->cx()) << 16) | cpu_->dx());
            const int whence = (cpu_->al() == 0) ? SEEK_SET : (cpu_->al() == 1 ? SEEK_CUR : SEEK_END);
            std::fseek(it->second.fp, offset, whence);
            const long pos = std::ftell(it->second.fp);
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            cpu_->set_ax(static_cast<uint16_t>(pos & 0xFFFF));
            cpu_->set_dx(static_cast<uint16_t>((pos >> 16) & 0xFFFF));
            return true;
        }
        case 0x44: // IOCTL
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            if (cpu_->al() == 0x00) {
                cpu_->set_dx((cpu_->bx() <= 2) ? 0x80D3 : 0x0000);
            }
            return true;
        case 0x48: { // Allocate Memory
            const uint16_t paras = cpu_->bx();
            if (static_cast<uint32_t>(next_alloc_seg_) + paras > 0x9F00u) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(8);
                cpu_->set_bx(static_cast<uint16_t>(0x9F00u - next_alloc_seg_));
            } else {
                cpu_->set_flag(Cpu386::FLAG_CF, false);
                cpu_->set_ax(next_alloc_seg_);
                next_alloc_seg_ = static_cast<uint16_t>(next_alloc_seg_ + paras);
            }
            return true;
        }
        case 0x49:
        case 0x4A:
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            return true;
        case 0x4C: // Exit
            std::fprintf(stderr, "[DOS] Program terminated with code %u\n", cpu_->al());
            cpu_->set_halted(true);
            return true;
        case 0x4E: { // FindFirst
            const std::string pat = read_asciiz(cpu_->sreg[Cpu386::SEG_DS], cpu_->dx());
            const uint16_t attr = cpu_->cx();
            if (trace_overlays_) std::fprintf(stderr, "[DOS] FindFirst %s attr=0x%04X\n", pat.c_str(), attr);
            if ((attr & 0x08) && to_lower(pat).rfind("f:", 0) == 0) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(18);
                return true;
            }
            find_results_.clear();
            find_index_ = 0;
            std::string clean = pat;
            if (clean.size() >= 2 && std::isalpha(static_cast<unsigned char>(clean[0])) && clean[1] == ':') {
                clean = clean.substr(2);
            }
            for (char& c : clean) if (c == '\\') c = '/';
            if (!clean.empty() && clean[0] == '/') clean = clean.substr(1);
            const std::string pat_low = to_lower(clean);
            std::error_code ec;
            for (const auto& e : fs::directory_iterator(data_dir_, ec)) {
                if (!e.is_regular_file(ec)) continue;
                const std::string fn = e.path().filename().string();
                if (wildcard_match(pat_low, to_lower(fn))) {
                    find_results_.push_back(fn);
                }
            }
            if (find_results_.empty()) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(18);
                return true;
            }
            write_dta_entry(find_results_[find_index_++]);
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            return true;
        }
        case 0x4F: { // FindNext
            if (find_index_ >= find_results_.size()) {
                cpu_->set_flag(Cpu386::FLAG_CF, true);
                cpu_->set_ax(18);
                return true;
            }
            write_dta_entry(find_results_[find_index_++]);
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            return true;
        }
        default:
            cpu_->set_flag(Cpu386::FLAG_CF, false);
            return true;
    }
}

bool DosRuntime::handle_int2f() {
    if (cpu_->ax() == 0x1500) {
        cpu_->set_bx(0); // No MSCDEX CD-ROM
    }
    return true;
}

bool DosRuntime::handle_int33() {
    const uint16_t fn = cpu_->ax();
    switch (fn) {
        case 0x0000:
            mouse_cb_mask_ = 0;
            mouse_cb_seg_ = 0;
            mouse_cb_off_ = 0;
            cpu_->set_ax(0xFFFF);
            cpu_->set_bx(2);
            break;
        case 0x0003: {
            const int rx = (mouse_max_x_ > mouse_min_x_)
                ? (mouse_min_x_ + (mouse_x_ * (mouse_max_x_ - mouse_min_x_)) / 639)
                : mouse_x_;
            const int ry = (mouse_max_y_ > mouse_min_y_)
                ? (mouse_min_y_ + (mouse_y_ * (mouse_max_y_ - mouse_min_y_)) / 399)
                : mouse_y_;
            cpu_->set_bx(mouse_buttons_);
            cpu_->set_cx(static_cast<uint16_t>(rx));
            cpu_->set_dx(static_cast<uint16_t>(ry));
            break;
        }
        case 0x0004:
            if (mouse_max_x_ > 0) mouse_x_ = std::clamp<int>((cpu_->cx() * 639) / mouse_max_x_, 0, 639);
            if (mouse_max_y_ > 0) mouse_y_ = std::clamp<int>((cpu_->dx() * 399) / mouse_max_y_, 0, 399);
            break;
        case 0x0007:
            mouse_min_x_ = cpu_->cx();
            mouse_max_x_ = cpu_->dx();
            break;
        case 0x0008:
            mouse_min_y_ = cpu_->cx();
            mouse_max_y_ = cpu_->dx();
            break;
        case 0x000C:
            mouse_cb_mask_ = cpu_->cx();
            mouse_cb_seg_ = cpu_->sreg[Cpu386::SEG_ES];
            mouse_cb_off_ = cpu_->dx();
            if (trace_overlays_) {
                std::fprintf(stderr, "[MOUSE] INT 33h AX=000Ch mask=0x%04X cb=%04X:%04X\n",
                             mouse_cb_mask_, mouse_cb_seg_, mouse_cb_off_);
            }
            break;
        case 0x0014: {
            const uint16_t old_mask = mouse_cb_mask_;
            const uint16_t old_seg = mouse_cb_seg_;
            const uint16_t old_off = mouse_cb_off_;
            mouse_cb_mask_ = cpu_->cx();
            mouse_cb_seg_ = cpu_->sreg[Cpu386::SEG_ES];
            mouse_cb_off_ = cpu_->dx();
            cpu_->set_cx(old_mask);
            cpu_->sreg[Cpu386::SEG_ES] = old_seg;
            cpu_->set_dx(old_off);
            break;
        }
        default:
            break;
    }
    return true;
}

int DosRuntime::ems_free_pages() const {
    int used = 0;
    for (const auto& kv : ems_handles_) {
        if (kv.second.active) used += static_cast<int>(kv.second.pages.size());
    }
    return std::max(0, kEmsTotalPages - used);
}

void DosRuntime::ems_sync_out(int phys_page) {
    if (phys_page < 0 || phys_page >= 4) return;
    const int32_t h = ems_mapped_handle_[phys_page];
    const int32_t lp = ems_mapped_page_[phys_page];
    if (h < 0 || lp < 0) return;
    auto it = ems_handles_.find(static_cast<uint16_t>(h));
    if (it != ems_handles_.end() && it->second.active && static_cast<size_t>(lp) < it->second.pages.size()) {
        const uint32_t frame_phys = Cpu386::phys(kEmsPageFrameSeg, static_cast<uint32_t>(phys_page) * 16384u);
        std::memcpy(it->second.pages[static_cast<size_t>(lp)].data(), cpu_->ram() + frame_phys, 16384);
    }
}

bool DosRuntime::handle_int67() {
    const uint8_t ah = cpu_->ah();
    switch (ah) {
        case 0x40: // Get Status
            cpu_->set_ah(0);
            break;
        case 0x41: // Get Page Frame Segment
            cpu_->set_ah(0);
            cpu_->set_bx(kEmsPageFrameSeg);
            break;
        case 0x42: // Get Unallocated Page Count
            cpu_->set_ah(0);
            cpu_->set_bx(static_cast<uint16_t>(ems_free_pages()));
            cpu_->set_dx(static_cast<uint16_t>(kEmsTotalPages));
            break;
        case 0x43: { // Allocate Pages
            const uint16_t n = cpu_->bx();
            if (n == 0) {
                cpu_->set_ah(0x89);
                break;
            }
            if (n > ems_free_pages()) {
                cpu_->set_ah(0x87);
                break;
            }
            const uint16_t h = next_ems_handle_++;
            EmsHandle eh;
            eh.active = true;
            eh.pages.resize(n);
            for (auto& pg : eh.pages) pg.fill(0);
            ems_handles_[h] = std::move(eh);
            cpu_->set_ah(0);
            cpu_->set_dx(h);
            break;
        }
        case 0x44: { // Map / Unmap Handle Page
            const uint8_t phys_p = cpu_->al();
            const uint16_t log_p = cpu_->bx();
            const uint16_t h = cpu_->dx();
            if (phys_p >= 4) {
                cpu_->set_ah(0x8B);
                break;
            }
            ems_sync_out(phys_p);
            if (log_p == 0xFFFF) {
                ems_mapped_handle_[phys_p] = -1;
                ems_mapped_page_[phys_p] = -1;
                cpu_->set_ah(0);
                break;
            }
            auto it = ems_handles_.find(h);
            if (it == ems_handles_.end() || !it->second.active) {
                cpu_->set_ah(0x83);
                break;
            }
            if (log_p >= it->second.pages.size()) {
                cpu_->set_ah(0x8A);
                break;
            }
            const uint32_t frame_phys = Cpu386::phys(kEmsPageFrameSeg, static_cast<uint32_t>(phys_p) * 16384u);
            std::memcpy(cpu_->ram() + frame_phys, it->second.pages[log_p].data(), 16384);
            ems_mapped_handle_[phys_p] = h;
            ems_mapped_page_[phys_p] = log_p;
            cpu_->set_ah(0);
            break;
        }
        case 0x45: { // Deallocate Pages
            const uint16_t h = cpu_->dx();
            for (int p = 0; p < 4; ++p) {
                if (ems_mapped_handle_[p] == h) {
                    ems_mapped_handle_[p] = -1;
                    ems_mapped_page_[p] = -1;
                }
            }
            ems_handles_.erase(h);
            cpu_->set_ah(0);
            break;
        }
        case 0x46: // Get Version
            cpu_->set_ax(0x0040);
            break;
        case 0x47: // Save Page Map
        case 0x48: // Restore Page Map
            cpu_->set_ah(0);
            break;
        case 0x4B: // Handle Count
            cpu_->set_ah(0);
            cpu_->set_bx(static_cast<uint16_t>(ems_handles_.size()));
            break;
        case 0x4C: { // Get Pages for Handle
            auto it = ems_handles_.find(cpu_->dx());
            if (it == ems_handles_.end()) {
                cpu_->set_ah(0x83);
            } else {
                cpu_->set_ah(0);
                cpu_->set_bx(static_cast<uint16_t>(it->second.pages.size()));
            }
            break;
        }
        case 0x51: { // Reallocate Pages
            auto it = ems_handles_.find(cpu_->dx());
            if (it == ems_handles_.end()) {
                cpu_->set_ah(0x83);
            } else {
                for (int p = 0; p < 4; ++p) {
                    if (ems_mapped_handle_[p] == cpu_->dx()) ems_sync_out(p);
                }
                it->second.pages.resize(cpu_->bx());
                cpu_->set_ah(0);
                cpu_->set_bx(cpu_->bx());
            }
            break;
        }
        default:
            cpu_->set_ah(0);
            break;
    }
    return true;
}

uint16_t DosRuntime::port_in(uint16_t port, int size) {
    if (port >= 0x03C0 && port <= 0x03DF) {
        return vga_->in8(port);
    }
    switch (port) {
        case 0x0060:
            return pop_scancode_for_irq();
        case 0x0061:
            return 0x00;
        case 0x0064:
            return kbd_scancodes_.empty() ? 0x00 : 0x01;
        case 0x0020:
        case 0x0021:
            return 0x00;
        case 0x0040:
        case 0x0041:
        case 0x0042:
            return 0x00;
        case 0x022A:
            return sb_read_byte_;
        case 0x022C:
            return 0x00; // SB DSP write buffer ready
        case 0x022E:
            return 0x80; // SB DSP data available
        default:
            return (size == 2) ? 0xFFFF : 0xFF;
    }
}

void DosRuntime::port_out(uint16_t port, uint16_t val, int size) {
    if (port >= 0x03C0 && port <= 0x03DF) {
        if (size == 2) vga_->out16(port, val);
        else vga_->out8(port, static_cast<uint8_t>(val));
        return;
    }
    if (port == 0x0226 && (val & 1) == 0) {
        sb_read_byte_ = 0xAA;
    }
}
