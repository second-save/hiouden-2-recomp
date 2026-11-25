#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

class EgaVga;

class Cpu386 {
public:
    enum Seg {
        SEG_ES = 0,
        SEG_CS = 1,
        SEG_SS = 2,
        SEG_DS = 3,
        SEG_FS = 4,
        SEG_GS = 5
    };

    enum Flag : uint32_t {
        FLAG_CF = 1u << 0,
        FLAG_PF = 1u << 2,
        FLAG_AF = 1u << 4,
        FLAG_ZF = 1u << 6,
        FLAG_SF = 1u << 7,
        FLAG_TF = 1u << 8,
        FLAG_IF = 1u << 9,
        FLAG_DF = 1u << 10,
        FLAG_OF = 1u << 11,
    };

    using IntHandler = std::function<bool(uint8_t vec)>;
    using PortInHandler = std::function<uint16_t(uint16_t port, int size)>;
    using PortOutHandler = std::function<void(uint16_t port, uint16_t val, int size)>;

    explicit Cpu386(EgaVga* vga);

    void reset();

    void set_int_handler(IntHandler cb) { int_handler_ = std::move(cb); }
    void set_port_handlers(PortInHandler in_cb, PortOutHandler out_cb) {
        port_in_ = std::move(in_cb);
        port_out_ = std::move(out_cb);
    }

    // Registers (little-endian host: macOS arm64)
    uint32_t eax = 0, ecx = 0, edx = 0, ebx = 0;
    uint32_t esp = 0, ebp = 0, esi = 0, edi = 0;
    uint16_t sreg[6]{}; // ES, CS, SS, DS, FS, GS
    uint16_t ip = 0;
    uint32_t eflags = 0x0202; // IF=1

    // 8/16-bit register accessors
    uint8_t al() const { return static_cast<uint8_t>(eax & 0xFF); }
    uint8_t ah() const { return static_cast<uint8_t>((eax >> 8) & 0xFF); }
    uint8_t cl() const { return static_cast<uint8_t>(ecx & 0xFF); }
    uint8_t ch() const { return static_cast<uint8_t>((ecx >> 8) & 0xFF); }
    uint8_t dl() const { return static_cast<uint8_t>(edx & 0xFF); }
    uint8_t dh() const { return static_cast<uint8_t>((edx >> 8) & 0xFF); }
    uint8_t bl() const { return static_cast<uint8_t>(ebx & 0xFF); }
    uint8_t bh() const { return static_cast<uint8_t>((ebx >> 8) & 0xFF); }

    uint16_t ax() const { return static_cast<uint16_t>(eax); }
    uint16_t cx() const { return static_cast<uint16_t>(ecx); }
    uint16_t dx() const { return static_cast<uint16_t>(edx); }
    uint16_t bx() const { return static_cast<uint16_t>(ebx); }
    uint16_t sp() const { return static_cast<uint16_t>(esp); }
    uint16_t bp() const { return static_cast<uint16_t>(ebp); }
    uint16_t si() const { return static_cast<uint16_t>(esi); }
    uint16_t di() const { return static_cast<uint16_t>(edi); }

    void set_al(uint8_t v) { eax = (eax & ~0xFFu) | v; }
    void set_ah(uint8_t v) { eax = (eax & ~0xFF00u) | (static_cast<uint32_t>(v) << 8); }
    void set_ax(uint16_t v) { eax = (eax & ~0xFFFFu) | v; }
    void set_cx(uint16_t v) { ecx = (ecx & ~0xFFFFu) | v; }
    void set_dx(uint16_t v) { edx = (edx & ~0xFFFFu) | v; }
    void set_bx(uint16_t v) { ebx = (ebx & ~0xFFFFu) | v; }
    void set_sp(uint16_t v) { esp = (esp & ~0xFFFFu) | v; }
    void set_bp(uint16_t v) { ebp = (ebp & ~0xFFFFu) | v; }
    void set_si(uint16_t v) { esi = (esi & ~0xFFFFu) | v; }
    void set_di(uint16_t v) { edi = (edi & ~0xFFFFu) | v; }

    bool get_flag(Flag f) const { return (eflags & f) != 0; }
    void set_flag(Flag f, bool v) {
        if (v) eflags |= f;
        else eflags &= ~static_cast<uint32_t>(f);
    }

    // Physical memory
    uint8_t* ram() { return ram_.data(); }
    const uint8_t* ram() const { return ram_.data(); }

    static uint32_t phys(uint16_t seg, uint32_t off) {
        return ((static_cast<uint32_t>(seg) << 4) + (off & 0xFFFFu)) & 0xFFFFFu;
    }

    uint8_t read8(uint32_t addr);
    uint16_t read16(uint32_t addr);
    uint32_t read32(uint32_t addr);
    void write8(uint32_t addr, uint8_t val);
    void write16(uint32_t addr, uint16_t val);
    void write32(uint32_t addr, uint32_t val);

    uint8_t seg_read8(uint16_t seg, uint32_t off) { return read8(phys(seg, off)); }
    uint16_t seg_read16(uint16_t seg, uint32_t off) { return read16(phys(seg, off)); }
    uint32_t seg_read32(uint16_t seg, uint32_t off) { return read32(phys(seg, off)); }
    void seg_write8(uint16_t seg, uint32_t off, uint8_t val) { write8(phys(seg, off), val); }
    void seg_write16(uint16_t seg, uint32_t off, uint16_t val) { write16(phys(seg, off), val); }
    void seg_write32(uint16_t seg, uint32_t off, uint32_t val) { write32(phys(seg, off), val); }

    void push16(uint16_t val);
    uint16_t pop16();
    void push32(uint32_t val);
    uint32_t pop32();

    // Execute one instruction; returns false if halted or fatal error
    bool step();

    // Trigger hardware IRQ vector (only fires if IF=1)
    bool trigger_irq(uint8_t vec);
    // Execute software INT vector
    void raise_int(uint8_t vec);

    bool halted() const { return halted_; }
    void set_halted(bool h) { halted_ = h; }
    bool error() const { return error_; }
    uint64_t insn_count() const { return insn_count_; }

    // Spin/wait detection hints for main loop
    bool hit_vblank_poll() const { return hit_vblank_poll_; }
    void clear_vblank_poll() { hit_vblank_poll_ = false; }
    bool hit_timer_read() const { return hit_timer_read_; }
    void clear_timer_read() { hit_timer_read_ = false; }

private:
    std::array<uint8_t, 1048576> ram_{};
    EgaVga* vga_ = nullptr;
    IntHandler int_handler_;
    PortInHandler port_in_;
    PortOutHandler port_out_;

    bool halted_ = false;
    bool error_ = false;
    uint64_t insn_count_ = 0;

    bool hit_vblank_poll_ = false;
    bool hit_timer_read_ = false;

    // Per-instruction decoded state
    int seg_override_ = -1;
    bool op32_ = false;
    bool addr32_ = false;
    int rep_mode_ = 0; // 0=none, 1=REPNE, 2=REP/REPE
    uint16_t cur_cs_ = 0;
    uint16_t cur_ip_ = 0;

    uint8_t fetch8();
    uint16_t fetch16();
    uint32_t fetch32();
    int8_t fetch_s8() { return static_cast<int8_t>(fetch8()); }
    int16_t fetch_s16() { return static_cast<int16_t>(fetch16()); }
    int32_t fetch_s32() { return static_cast<int32_t>(fetch32()); }

    // Register helpers
    uint32_t get_reg(int idx, int size) const;
    void set_reg(int idx, int size, uint32_t val);

    struct ModRM {
        uint8_t mod;
        uint8_t reg;
        uint8_t rm;
        bool is_mem;
        uint16_t seg;
        uint32_t off;
    };
    ModRM decode_modrm(bool is_32bit_addr);
    uint32_t read_rm(const ModRM& m, int size);
    void write_rm(const ModRM& m, int size, uint32_t val);

    // ALU helpers
    void set_szp(uint32_t res, int size);
    uint32_t alu_op(int op, uint32_t a, uint32_t b, int size);
    uint32_t shift_op(int op, uint32_t val, uint8_t count, int size);
    uint32_t inc_op(uint32_t val, int size);
    uint32_t dec_op(uint32_t val, int size);
    bool eval_cc(uint8_t cc) const;

    bool exec_0f();
    bool exec_string_op(uint8_t op);
    void unimplemented(const char* detail);
};
