#include "cpu/cpu386.h"
#include "hw/ega_vga.h"

#include <climits>
#include <cstdio>
#include <cstring>

namespace {

inline bool parity8(uint8_t v) {
    v ^= static_cast<uint8_t>(v >> 4);
    v ^= static_cast<uint8_t>(v >> 2);
    v ^= static_cast<uint8_t>(v >> 1);
    return (v & 1u) == 0;
}

inline uint32_t size_mask(int size) {
    if (size == 1) return 0xFFu;
    if (size == 2) return 0xFFFFu;
    return 0xFFFFFFFFu;
}

inline uint32_t sign_bit(int size) {
    if (size == 1) return 0x80u;
    if (size == 2) return 0x8000u;
    return 0x80000000u;
}

inline int32_t sext(uint32_t v, int size) {
    if (size == 1) return static_cast<int8_t>(v & 0xFFu);
    if (size == 2) return static_cast<int16_t>(v & 0xFFFFu);
    return static_cast<int32_t>(v);
}

} // namespace

Cpu386::Cpu386(EgaVga* vga) : vga_(vga) {
    reset();
}

void Cpu386::reset() {
    ram_.fill(0);
    eax = ecx = edx = ebx = esp = ebp = esi = edi = 0;
    std::memset(sreg, 0, sizeof(sreg));
    ip = 0;
    eflags = 0x0202;
    halted_ = false;
    error_ = false;
    insn_count_ = 0;
    hit_vblank_poll_ = false;
    hit_timer_read_ = false;
}

uint8_t Cpu386::read8(uint32_t addr) {
    addr &= 0xFFFFFu;
    if (addr >= 0xA0000u && addr < 0xB0000u) {
        return vga_->vram_read8(addr - 0xA0000u);
    }
    if (addr == 0x00450u || addr == 0x00451u) {
        hit_timer_read_ = true;
    }
    return ram_[addr];
}

uint16_t Cpu386::read16(uint32_t addr) {
    addr &= 0xFFFFFu;
    if (addr >= 0xA0000u && addr < 0xB0000u) {
        const uint8_t lo = vga_->vram_read8(addr - 0xA0000u);
        const uint8_t hi = vga_->vram_read8(((addr + 1) & 0xFFFFFu) - 0xA0000u);
        return static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
    }
    if (addr == 0x00450u) {
        hit_timer_read_ = true;
    }
    const uint8_t lo = ram_[addr];
    const uint8_t hi = ram_[(addr + 1) & 0xFFFFFu];
    return static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
}

uint32_t Cpu386::read32(uint32_t addr) {
    const uint16_t lo = read16(addr);
    const uint16_t hi = read16((addr + 2) & 0xFFFFFu);
    return static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
}

void Cpu386::write8(uint32_t addr, uint8_t val) {
    addr &= 0xFFFFFu;
    if (addr >= 0xA0000u && addr < 0xB0000u) {
        vga_->vram_write8(addr - 0xA0000u, val);
        return;
    }
    ram_[addr] = val;
}

void Cpu386::write16(uint32_t addr, uint16_t val) {
    addr &= 0xFFFFFu;
    if (addr >= 0xA0000u && addr < 0xB0000u) {
        vga_->vram_write8(addr - 0xA0000u, static_cast<uint8_t>(val & 0xFFu));
        vga_->vram_write8(((addr + 1) & 0xFFFFFu) - 0xA0000u, static_cast<uint8_t>(val >> 8));
        return;
    }
    ram_[addr] = static_cast<uint8_t>(val & 0xFFu);
    ram_[(addr + 1) & 0xFFFFFu] = static_cast<uint8_t>(val >> 8);
}

void Cpu386::write32(uint32_t addr, uint32_t val) {
    write16(addr, static_cast<uint16_t>(val & 0xFFFFu));
    write16((addr + 2) & 0xFFFFFu, static_cast<uint16_t>(val >> 16));
}

void Cpu386::push16(uint16_t val) {
    set_sp(static_cast<uint16_t>(sp() - 2));
    seg_write16(sreg[SEG_SS], sp(), val);
}

uint16_t Cpu386::pop16() {
    const uint16_t val = seg_read16(sreg[SEG_SS], sp());
    set_sp(static_cast<uint16_t>(sp() + 2));
    return val;
}

void Cpu386::push32(uint32_t val) {
    set_sp(static_cast<uint16_t>(sp() - 4));
    seg_write32(sreg[SEG_SS], sp(), val);
}

uint32_t Cpu386::pop32() {
    const uint32_t val = seg_read32(sreg[SEG_SS], sp());
    set_sp(static_cast<uint16_t>(sp() + 4));
    return val;
}

uint8_t Cpu386::fetch8() {
    const uint8_t v = seg_read8(sreg[SEG_CS], ip);
    ip = static_cast<uint16_t>(ip + 1);
    return v;
}

uint16_t Cpu386::fetch16() {
    const uint8_t lo = fetch8();
    const uint8_t hi = fetch8();
    return static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
}

uint32_t Cpu386::fetch32() {
    const uint16_t lo = fetch16();
    const uint16_t hi = fetch16();
    return static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
}

uint32_t Cpu386::get_reg(int idx, int size) const {
    if (size == 1) {
        switch (idx & 7) {
            case 0: return al();
            case 1: return cl();
            case 2: return dl();
            case 3: return bl();
            case 4: return ah();
            case 5: return ch();
            case 6: return dh();
            case 7: return bh();
        }
    }
    uint32_t* regs[8] = {
        const_cast<uint32_t*>(&eax), const_cast<uint32_t*>(&ecx),
        const_cast<uint32_t*>(&edx), const_cast<uint32_t*>(&ebx),
        const_cast<uint32_t*>(&esp), const_cast<uint32_t*>(&ebp),
        const_cast<uint32_t*>(&esi), const_cast<uint32_t*>(&edi)
    };
    const uint32_t val = *regs[idx & 7];
    return (size == 2) ? (val & 0xFFFFu) : val;
}

void Cpu386::set_reg(int idx, int size, uint32_t val) {
    uint32_t* regs[8] = {&eax, &ecx, &edx, &ebx, &esp, &ebp, &esi, &edi};
    if (size == 1) {
        const int r = idx & 3;
        if (idx & 4) {
            *regs[r] = (*regs[r] & ~0xFF00u) | ((val & 0xFFu) << 8);
        } else {
            *regs[r] = (*regs[r] & ~0xFFu) | (val & 0xFFu);
        }
    } else if (size == 2) {
        *regs[idx & 7] = (*regs[idx & 7] & ~0xFFFFu) | (val & 0xFFFFu);
    } else {
        *regs[idx & 7] = val;
    }
}

Cpu386::ModRM Cpu386::decode_modrm(bool is_32bit_addr) {
    const uint8_t b = fetch8();
    ModRM m{};
    m.mod = (b >> 6) & 3;
    m.reg = (b >> 3) & 7;
    m.rm = b & 7;
    if (m.mod == 3) {
        m.is_mem = false;
        return m;
    }
    m.is_mem = true;
    int def_seg = SEG_DS;

    if (!is_32bit_addr) {
        uint16_t off = 0;
        if (m.mod == 0 && m.rm == 6) {
            off = fetch16();
        } else {
            switch (m.rm) {
                case 0: off = static_cast<uint16_t>(bx() + si()); break;
                case 1: off = static_cast<uint16_t>(bx() + di()); break;
                case 2: off = static_cast<uint16_t>(bp() + si()); def_seg = SEG_SS; break;
                case 3: off = static_cast<uint16_t>(bp() + di()); def_seg = SEG_SS; break;
                case 4: off = si(); break;
                case 5: off = di(); break;
                case 6: off = bp(); def_seg = SEG_SS; break;
                case 7: off = bx(); break;
            }
            if (m.mod == 1) {
                off = static_cast<uint16_t>(off + fetch_s8());
            } else if (m.mod == 2) {
                off = static_cast<uint16_t>(off + fetch16());
            }
        }
        m.seg = sreg[seg_override_ >= 0 ? seg_override_ : def_seg];
        m.off = off;
        return m;
    }

    // 32-bit addressing mode (0x67 prefix)
    uint32_t off = 0;
    if (m.rm == 4) {
        const uint8_t sib = fetch8();
        const uint8_t scale = 1u << ((sib >> 6) & 3);
        const uint8_t index = (sib >> 3) & 7;
        const uint8_t base = sib & 7;
        uint32_t idx_val = (index == 4) ? 0 : get_reg(index, 4);
        uint32_t base_val = 0;
        if (base == 5 && m.mod == 0) {
            base_val = fetch32();
        } else {
            base_val = get_reg(base, 4);
            if (base == 4 || base == 5) def_seg = SEG_SS;
        }
        off = base_val + idx_val * scale;
    } else if (m.mod == 0 && m.rm == 5) {
        off = fetch32();
    } else {
        off = get_reg(m.rm, 4);
        if (m.rm == 5) def_seg = SEG_SS;
    }
    if (m.mod == 1) {
        off += static_cast<uint32_t>(static_cast<int32_t>(fetch_s8()));
    } else if (m.mod == 2) {
        off += fetch32();
    }
    m.seg = sreg[seg_override_ >= 0 ? seg_override_ : def_seg];
    m.off = off & 0xFFFFu;
    return m;
}

uint32_t Cpu386::read_rm(const ModRM& m, int size) {
    if (!m.is_mem) return get_reg(m.rm, size);
    if (size == 1) return seg_read8(m.seg, m.off);
    if (size == 2) return seg_read16(m.seg, m.off);
    return seg_read32(m.seg, m.off);
}

void Cpu386::write_rm(const ModRM& m, int size, uint32_t val) {
    if (!m.is_mem) {
        set_reg(m.rm, size, val);
        return;
    }
    if (size == 1) seg_write8(m.seg, m.off, static_cast<uint8_t>(val));
    else if (size == 2) seg_write16(m.seg, m.off, static_cast<uint16_t>(val));
    else seg_write32(m.seg, m.off, val);
}

void Cpu386::set_szp(uint32_t res, int size) {
    const uint32_t mask = size_mask(size);
    const uint32_t r = res & mask;
    set_flag(FLAG_ZF, r == 0);
    set_flag(FLAG_SF, (r & sign_bit(size)) != 0);
    set_flag(FLAG_PF, parity8(static_cast<uint8_t>(r & 0xFFu)));
}

uint32_t Cpu386::alu_op(int op, uint32_t a, uint32_t b, int size) {
    const uint32_t mask = size_mask(size);
    const uint32_t sb = sign_bit(size);
    a &= mask;
    b &= mask;
    uint64_t res = 0;

    switch (op) {
        case 0: // ADD
        case 2: { // ADC
            const uint32_t c = (op == 2 && get_flag(FLAG_CF)) ? 1u : 0u;
            res = static_cast<uint64_t>(a) + b + c;
            const uint32_t r = static_cast<uint32_t>(res) & mask;
            set_flag(FLAG_CF, (res >> (size * 8)) != 0);
            set_flag(FLAG_AF, ((a ^ b ^ r) & 0x10u) != 0);
            set_flag(FLAG_OF, (((a ^ r) & (b ^ r)) & sb) != 0);
            set_szp(r, size);
            return r;
        }
        case 1: // OR
            res = a | b;
            set_flag(FLAG_CF, false);
            set_flag(FLAG_OF, false);
            set_flag(FLAG_AF, false);
            set_szp(static_cast<uint32_t>(res), size);
            return static_cast<uint32_t>(res);
        case 4: // AND
            res = a & b;
            set_flag(FLAG_CF, false);
            set_flag(FLAG_OF, false);
            set_flag(FLAG_AF, false);
            set_szp(static_cast<uint32_t>(res), size);
            return static_cast<uint32_t>(res);
        case 6: // XOR
            res = a ^ b;
            set_flag(FLAG_CF, false);
            set_flag(FLAG_OF, false);
            set_flag(FLAG_AF, false);
            set_szp(static_cast<uint32_t>(res), size);
            return static_cast<uint32_t>(res);
        case 3: // SBB
        case 5: // SUB
        case 7: { // CMP
            const uint32_t c = (op == 3 && get_flag(FLAG_CF)) ? 1u : 0u;
            const uint64_t sub = static_cast<uint64_t>(b) + c;
            res = static_cast<uint64_t>(a) - sub;
            const uint32_t r = static_cast<uint32_t>(res) & mask;
            set_flag(FLAG_CF, static_cast<uint64_t>(a) < sub);
            set_flag(FLAG_AF, ((a ^ b ^ r) & 0x10u) != 0);
            set_flag(FLAG_OF, (((a ^ b) & (a ^ r)) & sb) != 0);
            set_szp(r, size);
            return (op == 7) ? a : r;
        }
        default:
            return a;
    }
}

uint32_t Cpu386::inc_op(uint32_t val, int size) {
    const uint32_t mask = size_mask(size);
    const uint32_t sb = sign_bit(size);
    const uint32_t a = val & mask;
    const uint32_t r = (a + 1) & mask;
    set_flag(FLAG_AF, (r & 0x0Fu) == 0);
    set_flag(FLAG_OF, a == (sb - 1));
    set_szp(r, size);
    return r;
}

uint32_t Cpu386::dec_op(uint32_t val, int size) {
    const uint32_t mask = size_mask(size);
    const uint32_t sb = sign_bit(size);
    const uint32_t a = val & mask;
    const uint32_t r = (a - 1) & mask;
    set_flag(FLAG_AF, (a & 0x0Fu) == 0);
    set_flag(FLAG_OF, a == sb);
    set_szp(r, size);
    return r;
}

uint32_t Cpu386::shift_op(int op, uint32_t val, uint8_t count, int size) {
    count &= 0x1Fu;
    const uint32_t mask = size_mask(size);
    const int bits = size * 8;
    const uint32_t sb = sign_bit(size);
    uint32_t v = val & mask;
    if (count == 0) return v;

    bool cf = get_flag(FLAG_CF);
    bool of = get_flag(FLAG_OF);

    switch (op) {
        case 0: { // ROL
            const uint8_t c = static_cast<uint8_t>(count % bits);
            if (c != 0) {
                v = ((v << c) | (v >> (bits - c))) & mask;
            }
            cf = (v & 1u) != 0;
            of = ((v & sb) != 0) != cf;
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, of);
            return v;
        }
        case 1: { // ROR
            const uint8_t c = static_cast<uint8_t>(count % bits);
            if (c != 0) {
                v = ((v >> c) | (v << (bits - c))) & mask;
            }
            cf = (v & sb) != 0;
            of = ((v & sb) != 0) != ((v & (sb >> 1)) != 0);
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, of);
            return v;
        }
        case 2: { // RCL
            const uint8_t c = static_cast<uint8_t>(count % (bits + 1));
            for (uint8_t i = 0; i < c; ++i) {
                const bool new_cf = (v & sb) != 0;
                v = ((v << 1) | (cf ? 1u : 0u)) & mask;
                cf = new_cf;
            }
            of = ((v & sb) != 0) != cf;
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, of);
            return v;
        }
        case 3: { // RCR
            const uint8_t orig_msb = (v & sb) ? 1 : 0;
            const uint8_t c = static_cast<uint8_t>(count % (bits + 1));
            if (count == 1) {
                of = (orig_msb != (cf ? 1 : 0));
            }
            for (uint8_t i = 0; i < c; ++i) {
                const bool new_cf = (v & 1u) != 0;
                v = (v >> 1) | (cf ? sb : 0u);
                cf = new_cf;
            }
            if (c > 0 && count != 1) {
                of = ((v & sb) != 0) != ((v & (sb >> 1)) != 0);
            }
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, of);
            return v & mask;
        }
        case 4: // SHL / SAL
        case 6: {
            if (count <= bits) {
                cf = ((v >> (bits - count)) & 1u) != 0;
                v = (count == bits) ? 0u : ((v << count) & mask);
            } else {
                cf = false;
                v = 0;
            }
            of = ((v & sb) != 0) != cf;
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, of);
            set_szp(v, size);
            return v;
        }
        case 5: { // SHR
            if (count == 1) of = (v & sb) != 0;
            if (count <= bits) {
                cf = ((v >> (count - 1)) & 1u) != 0;
                v = (count == bits) ? 0u : (v >> count);
            } else {
                cf = false;
                v = 0;
            }
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, of);
            set_szp(v, size);
            return v & mask;
        }
        case 7: { // SAR
            const int32_t sv = sext(v, size);
            if (count >= bits) {
                cf = sv < 0;
                v = (sv < 0) ? mask : 0u;
            } else {
                cf = ((sv >> (count - 1)) & 1) != 0;
                v = static_cast<uint32_t>(sv >> count) & mask;
            }
            set_flag(FLAG_CF, cf);
            set_flag(FLAG_OF, false);
            set_szp(v, size);
            return v;
        }
        default:
            return v;
    }
}

bool Cpu386::eval_cc(uint8_t cc) const {
    const bool cf = get_flag(FLAG_CF);
    const bool zf = get_flag(FLAG_ZF);
    const bool sf = get_flag(FLAG_SF);
    const bool of = get_flag(FLAG_OF);
    const bool pf = get_flag(FLAG_PF);
    switch (cc & 0x0F) {
        case 0x0: return of;
        case 0x1: return !of;
        case 0x2: return cf;
        case 0x3: return !cf;
        case 0x4: return zf;
        case 0x5: return !zf;
        case 0x6: return cf || zf;
        case 0x7: return !cf && !zf;
        case 0x8: return sf;
        case 0x9: return !sf;
        case 0xA: return pf;
        case 0xB: return !pf;
        case 0xC: return sf != of;
        case 0xD: return sf == of;
        case 0xE: return zf || (sf != of);
        case 0xF: return !zf && (sf == of);
        default: return false;
    }
}

void Cpu386::raise_int(uint8_t vec) {
    if (int_handler_ && int_handler_(vec)) {
        return;
    }
    push16(static_cast<uint16_t>(eflags & 0xFFFFu));
    push16(sreg[SEG_CS]);
    push16(ip);
    set_flag(FLAG_IF, false);
    set_flag(FLAG_TF, false);
    const uint32_t ivt = static_cast<uint32_t>(vec) * 4u;
    ip = read16(ivt);
    sreg[SEG_CS] = read16(ivt + 2);
}

bool Cpu386::trigger_irq(uint8_t vec) {
    if (!get_flag(FLAG_IF)) return false;
    halted_ = false;
    raise_int(vec);
    return true;
}

void Cpu386::unimplemented(const char* detail) {
    std::fprintf(stderr,
                 "[CPU386] Unimplemented %s at CS:IP=%04X:%04X (phys %05X), bytes: %02X %02X %02X %02X\n",
                 detail, cur_cs_, cur_ip_, phys(cur_cs_, cur_ip_),
                 seg_read8(cur_cs_, cur_ip_),
                 seg_read8(cur_cs_, cur_ip_ + 1),
                 seg_read8(cur_cs_, cur_ip_ + 2),
                 seg_read8(cur_cs_, cur_ip_ + 3));
    error_ = true;
    halted_ = true;
}

bool Cpu386::exec_string_op(uint8_t op) {
    const int size = (op & 1) ? (op32_ ? 4 : 2) : 1;
    const int32_t step = get_flag(FLAG_DF) ? -size : size;
    const uint16_t src_seg = sreg[seg_override_ >= 0 ? seg_override_ : SEG_DS];
    const bool is_cmps_scas = ((op & 0xFE) == 0xA6) || ((op & 0xFE) == 0xAE);

    uint32_t count = (rep_mode_ != 0) ? (addr32_ ? ecx : cx()) : 1;
    while (count > 0) {
        switch (op & 0xFE) {
            case 0xA4: { // MOVS
                const uint32_t s = addr32_ ? esi : si();
                const uint32_t d = addr32_ ? edi : di();
                if (size == 1) seg_write8(sreg[SEG_ES], d, seg_read8(src_seg, s));
                else if (size == 2) seg_write16(sreg[SEG_ES], d, seg_read16(src_seg, s));
                else seg_write32(sreg[SEG_ES], d, seg_read32(src_seg, s));
                if (addr32_) { esi += step; edi += step; }
                else { set_si(static_cast<uint16_t>(si() + step)); set_di(static_cast<uint16_t>(di() + step)); }
                break;
            }
            case 0xA6: { // CMPS
                const uint32_t s = addr32_ ? esi : si();
                const uint32_t d = addr32_ ? edi : di();
                uint32_t a = 0, b = 0;
                if (size == 1) { a = seg_read8(src_seg, s); b = seg_read8(sreg[SEG_ES], d); }
                else if (size == 2) { a = seg_read16(src_seg, s); b = seg_read16(sreg[SEG_ES], d); }
                else { a = seg_read32(src_seg, s); b = seg_read32(sreg[SEG_ES], d); }
                alu_op(7, a, b, size);
                if (addr32_) { esi += step; edi += step; }
                else { set_si(static_cast<uint16_t>(si() + step)); set_di(static_cast<uint16_t>(di() + step)); }
                break;
            }
            case 0xAA: { // STOS
                const uint32_t d = addr32_ ? edi : di();
                if (size == 1) seg_write8(sreg[SEG_ES], d, al());
                else if (size == 2) seg_write16(sreg[SEG_ES], d, ax());
                else seg_write32(sreg[SEG_ES], d, eax);
                if (addr32_) edi += step;
                else set_di(static_cast<uint16_t>(di() + step));
                break;
            }
            case 0xAC: { // LODS
                const uint32_t s = addr32_ ? esi : si();
                if (size == 1) set_al(seg_read8(src_seg, s));
                else if (size == 2) set_ax(seg_read16(src_seg, s));
                else eax = seg_read32(src_seg, s);
                if (addr32_) esi += step;
                else set_si(static_cast<uint16_t>(si() + step));
                break;
            }
            case 0xAE: { // SCAS
                const uint32_t d = addr32_ ? edi : di();
                uint32_t a = (size == 1) ? al() : (size == 2 ? ax() : eax);
                uint32_t b = 0;
                if (size == 1) b = seg_read8(sreg[SEG_ES], d);
                else if (size == 2) b = seg_read16(sreg[SEG_ES], d);
                else b = seg_read32(sreg[SEG_ES], d);
                alu_op(7, a, b, size);
                if (addr32_) edi += step;
                else set_di(static_cast<uint16_t>(di() + step));
                break;
            }
            default:
                return false;
        }
        --count;
        if (rep_mode_ != 0 && is_cmps_scas) {
            if (rep_mode_ == 2 && !get_flag(FLAG_ZF)) break;
            if (rep_mode_ == 1 && get_flag(FLAG_ZF)) break;
        }
    }
    if (rep_mode_ != 0) {
        if (addr32_) ecx = count;
        else set_cx(static_cast<uint16_t>(count));
    }
    return true;
}

bool Cpu386::exec_0f() {
    const uint8_t op2 = fetch8();
    const int wsize = op32_ ? 4 : 2;
    const int bits = wsize * 8;

    if (op2 >= 0x80 && op2 <= 0x8F) { // Jcc rel16/rel32
        const int32_t disp = op32_ ? fetch_s32() : static_cast<int32_t>(fetch_s16());
        if (eval_cc(op2 - 0x80)) {
            ip = static_cast<uint16_t>(ip + disp);
        }
        return true;
    }
    if (op2 >= 0x90 && op2 <= 0x9F) { // SETcc r/m8
        const ModRM m = decode_modrm(addr32_);
        write_rm(m, 1, eval_cc(op2 - 0x90) ? 1u : 0u);
        return true;
    }

    switch (op2) {
        case 0xA0: push16(sreg[SEG_FS]); return true;
        case 0xA1: sreg[SEG_FS] = pop16(); return true;
        case 0xA8: push16(sreg[SEG_GS]); return true;
        case 0xA9: sreg[SEG_GS] = pop16(); return true;

        case 0xA3: // BT r/m, r
        case 0xAB: // BTS r/m, r
        case 0xB3: // BTR r/m, r
        case 0xBB: { // BTC r/m, r
            ModRM m = decode_modrm(addr32_);
            const int32_t bit = sext(get_reg(m.reg, wsize), wsize);
            if (!m.is_mem) {
                const uint32_t b = static_cast<uint32_t>(bit) & static_cast<uint32_t>(bits - 1);
                uint32_t v = read_rm(m, wsize);
                set_flag(FLAG_CF, ((v >> b) & 1u) != 0);
                if (op2 == 0xAB) v |= (1u << b);
                else if (op2 == 0xB3) v &= ~(1u << b);
                else if (op2 == 0xBB) v ^= (1u << b);
                write_rm(m, wsize, v);
            } else {
                const int32_t byte_off = (bit >= 0) ? (bit / 8) : ((bit - 7) / 8);
                const uint8_t b = static_cast<uint8_t>(bit - byte_off * 8);
                m.off = (m.off + static_cast<uint32_t>(byte_off)) & 0xFFFFu;
                uint8_t v = seg_read8(m.seg, m.off);
                set_flag(FLAG_CF, ((v >> b) & 1u) != 0);
                if (op2 == 0xAB) v = static_cast<uint8_t>(v | (1u << b));
                else if (op2 == 0xB3) v = static_cast<uint8_t>(v & ~(1u << b));
                else if (op2 == 0xBB) v = static_cast<uint8_t>(v ^ (1u << b));
                seg_write8(m.seg, m.off, v);
            }
            return true;
        }
        case 0xBA: { // BT/BTS/BTR/BTC r/m, imm8
            ModRM m = decode_modrm(addr32_);
            const uint8_t imm = fetch8() & static_cast<uint8_t>(bits - 1);
            uint32_t v = read_rm(m, wsize);
            set_flag(FLAG_CF, ((v >> imm) & 1u) != 0);
            if (m.reg == 5) v |= (1u << imm);
            else if (m.reg == 6) v &= ~(1u << imm);
            else if (m.reg == 7) v ^= (1u << imm);
            if (m.reg != 4) write_rm(m, wsize, v);
            return true;
        }
        case 0xA4: // SHLD r/m, r, imm8
        case 0xA5: { // SHLD r/m, r, CL
            const ModRM m = decode_modrm(addr32_);
            const uint8_t cnt = (op2 == 0xA4 ? fetch8() : cl()) & 0x1Fu;
            const uint32_t dst = read_rm(m, wsize);
            const uint32_t src = get_reg(m.reg, wsize);
            if (cnt != 0) {
                const uint8_t c = cnt % bits;
                uint32_t res = (c == 0) ? dst : ((dst << c) | (src >> (bits - c)));
                res &= size_mask(wsize);
                set_flag(FLAG_CF, ((dst >> (bits - cnt)) & 1u) != 0);
                set_flag(FLAG_OF, ((res & sign_bit(wsize)) != 0) != get_flag(FLAG_CF));
                set_szp(res, wsize);
                write_rm(m, wsize, res);
            }
            return true;
        }
        case 0xAC: // SHRD r/m, r, imm8
        case 0xAD: { // SHRD r/m, r, CL
            const ModRM m = decode_modrm(addr32_);
            const uint8_t cnt = (op2 == 0xAC ? fetch8() : cl()) & 0x1Fu;
            const uint32_t dst = read_rm(m, wsize);
            const uint32_t src = get_reg(m.reg, wsize);
            if (cnt != 0) {
                const uint8_t c = cnt % bits;
                uint32_t res = (c == 0) ? dst : ((dst >> c) | (src << (bits - c)));
                res &= size_mask(wsize);
                set_flag(FLAG_CF, ((dst >> (cnt - 1)) & 1u) != 0);
                set_flag(FLAG_OF, ((res ^ dst) & sign_bit(wsize)) != 0);
                set_szp(res, wsize);
                write_rm(m, wsize, res);
            }
            return true;
        }
        case 0xAF: { // IMUL r, r/m
            const ModRM m = decode_modrm(addr32_);
            const int64_t a = sext(get_reg(m.reg, wsize), wsize);
            const int64_t b = sext(read_rm(m, wsize), wsize);
            const int64_t p = a * b;
            const uint32_t r = static_cast<uint32_t>(p) & size_mask(wsize);
            set_reg(m.reg, wsize, r);
            const bool ov = (p != sext(r, wsize));
            set_flag(FLAG_CF, ov);
            set_flag(FLAG_OF, ov);
            return true;
        }
        case 0xB2: { // LSS
            const ModRM m = decode_modrm(addr32_);
            const uint32_t off_val = (wsize == 4) ? seg_read32(m.seg, m.off) : seg_read16(m.seg, m.off);
            const uint16_t seg_val = seg_read16(m.seg, m.off + wsize);
            set_reg(m.reg, wsize, off_val);
            sreg[SEG_SS] = seg_val;
            return true;
        }
        case 0xB4: { // LFS
            const ModRM m = decode_modrm(addr32_);
            const uint32_t off_val = (wsize == 4) ? seg_read32(m.seg, m.off) : seg_read16(m.seg, m.off);
            const uint16_t seg_val = seg_read16(m.seg, m.off + wsize);
            set_reg(m.reg, wsize, off_val);
            sreg[SEG_FS] = seg_val;
            return true;
        }
        case 0xB5: { // LGS
            const ModRM m = decode_modrm(addr32_);
            const uint32_t off_val = (wsize == 4) ? seg_read32(m.seg, m.off) : seg_read16(m.seg, m.off);
            const uint16_t seg_val = seg_read16(m.seg, m.off + wsize);
            set_reg(m.reg, wsize, off_val);
            sreg[SEG_GS] = seg_val;
            return true;
        }
        case 0xB6: { // MOVZX r, r/m8
            const ModRM m = decode_modrm(addr32_);
            set_reg(m.reg, wsize, read_rm(m, 1) & 0xFFu);
            return true;
        }
        case 0xB7: { // MOVZX r32, r/m16
            const ModRM m = decode_modrm(addr32_);
            set_reg(m.reg, wsize, read_rm(m, 2) & 0xFFFFu);
            return true;
        }
        case 0xBE: { // MOVSX r, r/m8
            const ModRM m = decode_modrm(addr32_);
            const int32_t v = static_cast<int8_t>(read_rm(m, 1) & 0xFFu);
            set_reg(m.reg, wsize, static_cast<uint32_t>(v));
            return true;
        }
        case 0xBF: { // MOVSX r32, r/m16
            const ModRM m = decode_modrm(addr32_);
            const int32_t v = static_cast<int16_t>(read_rm(m, 2) & 0xFFFFu);
            set_reg(m.reg, wsize, static_cast<uint32_t>(v));
            return true;
        }
        case 0xBC: { // BSF
            const ModRM m = decode_modrm(addr32_);
            const uint32_t v = read_rm(m, wsize) & size_mask(wsize);
            set_flag(FLAG_ZF, v == 0);
            if (v != 0) {
                int idx = 0;
                while (((v >> idx) & 1u) == 0) ++idx;
                set_reg(m.reg, wsize, static_cast<uint32_t>(idx));
            }
            return true;
        }
        case 0xBD: { // BSR
            const ModRM m = decode_modrm(addr32_);
            const uint32_t v = read_rm(m, wsize) & size_mask(wsize);
            set_flag(FLAG_ZF, v == 0);
            if (v != 0) {
                int idx = bits - 1;
                while (((v >> idx) & 1u) == 0) --idx;
                set_reg(m.reg, wsize, static_cast<uint32_t>(idx));
            }
            return true;
        }
        default:
            unimplemented("0F opcode");
            return false;
    }
}

bool Cpu386::step() {
    if (halted_) return false;

    cur_cs_ = sreg[SEG_CS];
    cur_ip_ = ip;
    seg_override_ = -1;
    op32_ = false;
    addr32_ = false;
    rep_mode_ = 0;

    uint8_t op = 0;
    for (;;) {
        op = fetch8();
        if (op == 0x26) { seg_override_ = SEG_ES; continue; }
        if (op == 0x2E) { seg_override_ = SEG_CS; continue; }
        if (op == 0x36) { seg_override_ = SEG_SS; continue; }
        if (op == 0x3E) { seg_override_ = SEG_DS; continue; }
        if (op == 0x64) { seg_override_ = SEG_FS; continue; }
        if (op == 0x65) { seg_override_ = SEG_GS; continue; }
        if (op == 0x66) { op32_ = true; continue; }
        if (op == 0x67) { addr32_ = true; continue; }
        if (op == 0xF0) { continue; } // LOCK
        if (op == 0xF2) { rep_mode_ = 1; continue; }
        if (op == 0xF3) { rep_mode_ = 2; continue; }
        break;
    }

    ++insn_count_;
    const int wsize = op32_ ? 4 : 2;

    // 0x00..0x3F: standard ALU ops (op = (b >> 3) & 7)
    if ((op & 0xC0) == 0x00 && (op & 7) < 6) {
        const int alu = (op >> 3) & 7;
        const int sz = (op & 1) ? wsize : 1;
        if ((op & 7) < 4) {
            const ModRM m = decode_modrm(addr32_);
            const bool reg_dst = (op & 2) != 0;
            if (reg_dst) {
                const uint32_t res = alu_op(alu, get_reg(m.reg, sz), read_rm(m, sz), sz);
                if (alu != 7) set_reg(m.reg, sz, res);
            } else {
                const uint32_t res = alu_op(alu, read_rm(m, sz), get_reg(m.reg, sz), sz);
                if (alu != 7) write_rm(m, sz, res);
            }
        } else {
            const uint32_t imm = (sz == 1) ? fetch8() : (sz == 2 ? fetch16() : fetch32());
            const uint32_t res = alu_op(alu, get_reg(0, sz), imm, sz);
            if (alu != 7) set_reg(0, sz, res);
        }
        return true;
    }

    // String ops
    if ((op >= 0xA4 && op <= 0xA7) || (op >= 0xAA && op <= 0xAF)) {
        return exec_string_op(op);
    }

    // Jcc rel8
    if (op >= 0x70 && op <= 0x7F) {
        const int8_t rel = fetch_s8();
        if (eval_cc(op - 0x70)) {
            ip = static_cast<uint16_t>(ip + rel);
        }
        return true;
    }

    // INC r16/32
    if (op >= 0x40 && op <= 0x47) {
        const int r = op - 0x40;
        set_reg(r, wsize, inc_op(get_reg(r, wsize), wsize));
        return true;
    }
    // DEC r16/32
    if (op >= 0x48 && op <= 0x4F) {
        const int r = op - 0x48;
        set_reg(r, wsize, dec_op(get_reg(r, wsize), wsize));
        return true;
    }
    // PUSH r16/32
    if (op >= 0x50 && op <= 0x57) {
        const uint32_t v = get_reg(op - 0x50, wsize);
        if (op32_) push32(v);
        else push16(static_cast<uint16_t>(v));
        return true;
    }
    // POP r16/32
    if (op >= 0x58 && op <= 0x5F) {
        const uint32_t v = op32_ ? pop32() : pop16();
        set_reg(op - 0x58, wsize, v);
        return true;
    }
    // XCHG AX/EAX, r
    if (op >= 0x90 && op <= 0x97) {
        if (op != 0x90) {
            const int r = op - 0x90;
            const uint32_t a = get_reg(0, wsize);
            const uint32_t b = get_reg(r, wsize);
            set_reg(0, wsize, b);
            set_reg(r, wsize, a);
        }
        return true;
    }
    // MOV r8, imm8
    if (op >= 0xB0 && op <= 0xB7) {
        set_reg(op - 0xB0, 1, fetch8());
        return true;
    }
    // MOV r16/32, imm16/32
    if (op >= 0xB8 && op <= 0xBF) {
        const uint32_t imm = op32_ ? fetch32() : fetch16();
        set_reg(op - 0xB8, wsize, imm);
        return true;
    }

    switch (op) {
        case 0x06: push16(sreg[SEG_ES]); return true;
        case 0x07: sreg[SEG_ES] = pop16(); return true;
        case 0x0E: push16(sreg[SEG_CS]); return true;
        case 0x0F: return exec_0f();
        case 0x16: push16(sreg[SEG_SS]); return true;
        case 0x17: sreg[SEG_SS] = pop16(); return true;
        case 0x1E: push16(sreg[SEG_DS]); return true;
        case 0x1F: sreg[SEG_DS] = pop16(); return true;

        case 0x27: { // DAA
            const uint8_t old_al = al();
            const bool old_cf = get_flag(FLAG_CF);
            set_flag(FLAG_CF, false);
            if (((old_al & 0x0F) > 9) || get_flag(FLAG_AF)) {
                const uint16_t tmp = static_cast<uint16_t>(al() + 6);
                set_al(static_cast<uint8_t>(tmp));
                set_flag(FLAG_CF, old_cf || (tmp > 0xFF));
                set_flag(FLAG_AF, true);
            } else {
                set_flag(FLAG_AF, false);
            }
            if ((old_al > 0x99) || old_cf) {
                set_al(static_cast<uint8_t>(al() + 0x60));
                set_flag(FLAG_CF, true);
            }
            set_szp(al(), 1);
            return true;
        }
        case 0x2F: { // DAS
            const uint8_t old_al = al();
            const bool old_cf = get_flag(FLAG_CF);
            set_flag(FLAG_CF, false);
            if (((old_al & 0x0F) > 9) || get_flag(FLAG_AF)) {
                const uint16_t tmp = static_cast<uint16_t>(al() - 6);
                set_al(static_cast<uint8_t>(tmp));
                set_flag(FLAG_CF, old_cf || (tmp > 0xFF));
                set_flag(FLAG_AF, true);
            } else {
                set_flag(FLAG_AF, false);
            }
            if ((old_al > 0x99) || old_cf) {
                set_al(static_cast<uint8_t>(al() - 0x60));
                set_flag(FLAG_CF, true);
            }
            set_szp(al(), 1);
            return true;
        }
        case 0x37: // AAA
            if (((al() & 0x0F) > 9) || get_flag(FLAG_AF)) {
                set_ax(static_cast<uint16_t>(ax() + 0x106));
                set_flag(FLAG_AF, true);
                set_flag(FLAG_CF, true);
            } else {
                set_flag(FLAG_AF, false);
                set_flag(FLAG_CF, false);
            }
            set_al(al() & 0x0F);
            return true;
        case 0x3F: // AAS
            if (((al() & 0x0F) > 9) || get_flag(FLAG_AF)) {
                set_ax(static_cast<uint16_t>(ax() - 6));
                set_ah(static_cast<uint8_t>(ah() - 1));
                set_flag(FLAG_AF, true);
                set_flag(FLAG_CF, true);
            } else {
                set_flag(FLAG_AF, false);
                set_flag(FLAG_CF, false);
            }
            set_al(al() & 0x0F);
            return true;

        case 0x60: // PUSHA / PUSHAD
            if (op32_) {
                const uint32_t tmp = esp;
                push32(eax); push32(ecx); push32(edx); push32(ebx);
                push32(tmp); push32(ebp); push32(esi); push32(edi);
            } else {
                const uint16_t tmp = sp();
                push16(ax()); push16(cx()); push16(dx()); push16(bx());
                push16(tmp);  push16(bp()); push16(si()); push16(di());
            }
            return true;
        case 0x61: // POPA / POPAD
            if (op32_) {
                edi = pop32(); esi = pop32(); ebp = pop32();
                (void)pop32();
                ebx = pop32(); edx = pop32(); ecx = pop32(); eax = pop32();
            } else {
                set_di(pop16()); set_si(pop16()); set_bp(pop16());
                (void)pop16();
                set_bx(pop16()); set_dx(pop16()); set_cx(pop16()); set_ax(pop16());
            }
            return true;

        case 0x62: { // BOUND (decode and continue)
            (void)decode_modrm(addr32_);
            return true;
        }
        case 0x68: // PUSH imm16/32
            if (op32_) push32(fetch32());
            else push16(fetch16());
            return true;
        case 0x6A: { // PUSH imm8 (sign-extended)
            const int32_t v = fetch_s8();
            if (op32_) push32(static_cast<uint32_t>(v));
            else push16(static_cast<uint16_t>(v));
            return true;
        }
        case 0x69: // IMUL r, r/m, imm16/32
        case 0x6B: { // IMUL r, r/m, imm8
            const ModRM m = decode_modrm(addr32_);
            const int64_t a = sext(read_rm(m, wsize), wsize);
            const int64_t b = (op == 0x6B) ? fetch_s8() : (op32_ ? fetch_s32() : fetch_s16());
            const int64_t p = a * b;
            const uint32_t r = static_cast<uint32_t>(p) & size_mask(wsize);
            set_reg(m.reg, wsize, r);
            const bool ov = (p != sext(r, wsize));
            set_flag(FLAG_CF, ov);
            set_flag(FLAG_OF, ov);
            return true;
        }
        case 0x6C:
        case 0x6D: { // INSB / INSW
            const int sz = (op == 0x6C) ? 1 : 2;
            const int32_t st = get_flag(FLAG_DF) ? -sz : sz;
            uint32_t cnt = (rep_mode_ != 0) ? (addr32_ ? ecx : cx()) : 1;
            while (cnt > 0) {
                const uint16_t val = port_in_ ? port_in_(dx(), sz) : 0xFFFF;
                const uint32_t d = addr32_ ? edi : di();
                if (sz == 1) seg_write8(sreg[SEG_ES], d, static_cast<uint8_t>(val));
                else seg_write16(sreg[SEG_ES], d, val);
                if (addr32_) edi += st; else set_di(static_cast<uint16_t>(di() + st));
                --cnt;
            }
            if (rep_mode_ != 0) { if (addr32_) ecx = 0; else set_cx(0); }
            return true;
        }
        case 0x6E:
        case 0x6F: { // OUTSB / OUTSW
            const int sz = (op == 0x6E) ? 1 : 2;
            const int32_t st = get_flag(FLAG_DF) ? -sz : sz;
            const uint16_t src_seg = sreg[seg_override_ >= 0 ? seg_override_ : SEG_DS];
            uint32_t cnt = (rep_mode_ != 0) ? (addr32_ ? ecx : cx()) : 1;
            while (cnt > 0) {
                const uint32_t s = addr32_ ? esi : si();
                const uint16_t val = (sz == 1) ? seg_read8(src_seg, s) : seg_read16(src_seg, s);
                if (port_out_) port_out_(dx(), val, sz);
                if (addr32_) esi += st; else set_si(static_cast<uint16_t>(si() + st));
                --cnt;
            }
            if (rep_mode_ != 0) { if (addr32_) ecx = 0; else set_cx(0); }
            return true;
        }

        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83: {
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0x80 || op == 0x82) ? 1 : wsize;
            uint32_t imm = 0;
            if (op == 0x80 || op == 0x82) imm = fetch8();
            else if (op == 0x83) imm = static_cast<uint32_t>(static_cast<int32_t>(fetch_s8()));
            else imm = (wsize == 4) ? fetch32() : fetch16();
            const uint32_t res = alu_op(m.reg, read_rm(m, sz), imm, sz);
            if (m.reg != 7) write_rm(m, sz, res);
            return true;
        }
        case 0x84: // TEST r/m8, r8
        case 0x85: { // TEST r/m, r
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0x84) ? 1 : wsize;
            alu_op(4, read_rm(m, sz), get_reg(m.reg, sz), sz);
            return true;
        }
        case 0x86:
        case 0x87: { // XCHG r/m, r
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0x86) ? 1 : wsize;
            const uint32_t a = read_rm(m, sz);
            const uint32_t b = get_reg(m.reg, sz);
            write_rm(m, sz, b);
            set_reg(m.reg, sz, a);
            return true;
        }
        case 0x88:
        case 0x89: { // MOV r/m, r
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0x88) ? 1 : wsize;
            write_rm(m, sz, get_reg(m.reg, sz));
            return true;
        }
        case 0x8A:
        case 0x8B: { // MOV r, r/m
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0x8A) ? 1 : wsize;
            set_reg(m.reg, sz, read_rm(m, sz));
            return true;
        }
        case 0x8C: { // MOV r/m16, Sreg
            const ModRM m = decode_modrm(addr32_);
            write_rm(m, 2, sreg[m.reg % 6]);
            return true;
        }
        case 0x8D: { // LEA r, m
            const ModRM m = decode_modrm(addr32_);
            set_reg(m.reg, wsize, m.off);
            return true;
        }
        case 0x8E: { // MOV Sreg, r/m16
            const ModRM m = decode_modrm(addr32_);
            sreg[m.reg % 6] = static_cast<uint16_t>(read_rm(m, 2));
            return true;
        }
        case 0x8F: { // POP r/m
            const ModRM m = decode_modrm(addr32_);
            const uint32_t v = op32_ ? pop32() : pop16();
            write_rm(m, wsize, v);
            return true;
        }

        case 0x98: // CBW / CWDE
            if (op32_) eax = static_cast<uint32_t>(static_cast<int16_t>(ax()));
            else set_ax(static_cast<uint16_t>(static_cast<int8_t>(al())));
            return true;
        case 0x99: // CWD / CDQ
            if (op32_) edx = (static_cast<int32_t>(eax) < 0) ? 0xFFFFFFFFu : 0u;
            else set_dx((static_cast<int16_t>(ax()) < 0) ? 0xFFFFu : 0u);
            return true;
        case 0x9A: { // CALL ptr16:16
            const uint16_t new_ip = fetch16();
            const uint16_t new_cs = fetch16();
            push16(sreg[SEG_CS]);
            push16(ip);
            sreg[SEG_CS] = new_cs;
            ip = new_ip;
            return true;
        }
        case 0x9B: // WAIT / FWAIT
            return true;
        case 0x9C: // PUSHF / PUSHFD
            if (op32_) push32(eflags & 0x00FCFFFFu);
            else push16(static_cast<uint16_t>(eflags & 0xFFFFu));
            return true;
        case 0x9D: // POPF / POPFD
            if (op32_) eflags = (pop32() & 0x00FCFFFFu) | 0x0002u;
            else eflags = (eflags & 0xFFFF0000u) | (pop16() & 0x0FD5u) | 0x0002u;
            return true;
        case 0x9E: // SAHF
            eflags = (eflags & ~0xD5u) | (ah() & 0xD5u) | 0x02u;
            return true;
        case 0x9F: // LAHF
            set_ah(static_cast<uint8_t>((eflags & 0xD5u) | 0x02u));
            return true;

        case 0xA0: // MOV AL, moffs8
        case 0xA1: { // MOV AX/EAX, moffs
            const uint16_t seg = sreg[seg_override_ >= 0 ? seg_override_ : SEG_DS];
            const uint32_t off = addr32_ ? fetch32() : fetch16();
            if (op == 0xA0) set_al(seg_read8(seg, off));
            else if (op32_) eax = seg_read32(seg, off);
            else set_ax(seg_read16(seg, off));
            return true;
        }
        case 0xA2: // MOV moffs8, AL
        case 0xA3: { // MOV moffs, AX/EAX
            const uint16_t seg = sreg[seg_override_ >= 0 ? seg_override_ : SEG_DS];
            const uint32_t off = addr32_ ? fetch32() : fetch16();
            if (op == 0xA2) seg_write8(seg, off, al());
            else if (op32_) seg_write32(seg, off, eax);
            else seg_write16(seg, off, ax());
            return true;
        }
        case 0xA8: // TEST AL, imm8
            alu_op(4, al(), fetch8(), 1);
            return true;
        case 0xA9: { // TEST AX/EAX, imm
            const uint32_t imm = op32_ ? fetch32() : fetch16();
            alu_op(4, get_reg(0, wsize), imm, wsize);
            return true;
        }

        case 0xC0:
        case 0xC1: { // Shift/rotate r/m, imm8
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0xC0) ? 1 : wsize;
            const uint8_t cnt = fetch8();
            write_rm(m, sz, shift_op(m.reg, read_rm(m, sz), cnt, sz));
            return true;
        }
        case 0xC2: { // RET imm16
            const uint16_t adj = fetch16();
            ip = pop16();
            set_sp(static_cast<uint16_t>(sp() + adj));
            return true;
        }
        case 0xC3: // RET
            ip = pop16();
            return true;
        case 0xC4: // LES
        case 0xC5: { // LDS
            const ModRM m = decode_modrm(addr32_);
            const uint32_t off_val = op32_ ? seg_read32(m.seg, m.off) : seg_read16(m.seg, m.off);
            const uint16_t seg_val = seg_read16(m.seg, m.off + wsize);
            set_reg(m.reg, wsize, off_val);
            sreg[(op == 0xC4) ? SEG_ES : SEG_DS] = seg_val;
            return true;
        }
        case 0xC6:
        case 0xC7: { // MOV r/m, imm
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0xC6) ? 1 : wsize;
            const uint32_t imm = (sz == 1) ? fetch8() : (sz == 2 ? fetch16() : fetch32());
            write_rm(m, sz, imm);
            return true;
        }
        case 0xC8: { // ENTER
            const uint16_t alloc = fetch16();
            const uint8_t level = fetch8() & 0x1F;
            push16(bp());
            const uint16_t frame_temp = sp();
            if (level > 0) {
                for (uint8_t i = 1; i < level; ++i) {
                    set_bp(static_cast<uint16_t>(bp() - 2));
                    push16(seg_read16(sreg[SEG_SS], bp()));
                }
                push16(frame_temp);
            }
            set_bp(frame_temp);
            set_sp(static_cast<uint16_t>(sp() - alloc));
            return true;
        }
        case 0xC9: // LEAVE
            set_sp(bp());
            set_bp(pop16());
            return true;
        case 0xCA: { // RETF imm16
            const uint16_t adj = fetch16();
            ip = pop16();
            sreg[SEG_CS] = pop16();
            set_sp(static_cast<uint16_t>(sp() + adj));
            return true;
        }
        case 0xCB: // RETF
            ip = pop16();
            sreg[SEG_CS] = pop16();
            return true;
        case 0xCC: // INT 3
            raise_int(3);
            return true;
        case 0xCD: // INT imm8
            raise_int(fetch8());
            return true;
        case 0xCE: // INTO
            if (get_flag(FLAG_OF)) raise_int(4);
            return true;
        case 0xCF: // IRET / IRETD
            if (op32_) {
                ip = static_cast<uint16_t>(pop32());
                sreg[SEG_CS] = static_cast<uint16_t>(pop32());
                eflags = (pop32() & 0x00FCFFFFu) | 0x0002u;
            } else {
                ip = pop16();
                sreg[SEG_CS] = pop16();
                eflags = (eflags & 0xFFFF0000u) | (pop16() & 0x0FD5u) | 0x0002u;
            }
            return true;

        case 0xD0:
        case 0xD1:
        case 0xD2:
        case 0xD3: { // Shift/rotate r/m by 1 or CL
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op & 1) ? wsize : 1;
            const uint8_t cnt = (op >= 0xD2) ? cl() : 1;
            write_rm(m, sz, shift_op(m.reg, read_rm(m, sz), cnt, sz));
            return true;
        }
        case 0xD4: { // AAM
            const uint8_t base = fetch8();
            if (base == 0) { raise_int(0); return true; }
            const uint8_t a = al();
            set_ah(static_cast<uint8_t>(a / base));
            set_al(static_cast<uint8_t>(a % base));
            set_szp(al(), 1);
            return true;
        }
        case 0xD5: { // AAD
            const uint8_t base = fetch8();
            set_al(static_cast<uint8_t>(al() + ah() * base));
            set_ah(0);
            set_szp(al(), 1);
            return true;
        }
        case 0xD6: // SALC
            set_al(get_flag(FLAG_CF) ? 0xFFu : 0x00u);
            return true;
        case 0xD7: { // XLAT
            const uint16_t seg = sreg[seg_override_ >= 0 ? seg_override_ : SEG_DS];
            set_al(seg_read8(seg, static_cast<uint16_t>(bx() + al())));
            return true;
        }

        case 0xE0: // LOOPNE
        case 0xE1: // LOOPE
        case 0xE2: { // LOOP
            const int8_t rel = fetch_s8();
            const uint16_t c = static_cast<uint16_t>(cx() - 1);
            set_cx(c);
            const bool cond = (op == 0xE2) ? (c != 0)
                            : (op == 0xE1 ? (c != 0 && get_flag(FLAG_ZF))
                                          : (c != 0 && !get_flag(FLAG_ZF)));
            if (cond) ip = static_cast<uint16_t>(ip + rel);
            return true;
        }
        case 0xE3: { // JCXZ
            const int8_t rel = fetch_s8();
            const uint32_t c = addr32_ ? ecx : cx();
            if (c == 0) ip = static_cast<uint16_t>(ip + rel);
            return true;
        }
        case 0xE4: { // IN AL, imm8
            const uint16_t port = fetch8();
            set_al(static_cast<uint8_t>(port_in_ ? port_in_(port, 1) : 0xFF));
            return true;
        }
        case 0xE5: { // IN AX/EAX, imm8
            const uint16_t port = fetch8();
            const uint16_t v = port_in_ ? port_in_(port, 2) : 0xFFFF;
            if (op32_) eax = v; else set_ax(v);
            return true;
        }
        case 0xE6: { // OUT imm8, AL
            const uint16_t port = fetch8();
            if (port_out_) port_out_(port, al(), 1);
            return true;
        }
        case 0xE7: { // OUT imm8, AX/EAX
            const uint16_t port = fetch8();
            if (port_out_) port_out_(port, ax(), 2);
            return true;
        }
        case 0xE8: { // CALL rel16/32
            const int32_t rel = op32_ ? fetch_s32() : fetch_s16();
            if (op32_) push32(ip); else push16(ip);
            ip = static_cast<uint16_t>(ip + rel);
            return true;
        }
        case 0xE9: { // JMP rel16/32
            const int32_t rel = op32_ ? fetch_s32() : fetch_s16();
            ip = static_cast<uint16_t>(ip + rel);
            return true;
        }
        case 0xEA: { // JMP ptr16:16
            const uint16_t new_ip = fetch16();
            const uint16_t new_cs = fetch16();
            ip = new_ip;
            sreg[SEG_CS] = new_cs;
            return true;
        }
        case 0xEB: { // JMP rel8
            const int8_t rel = fetch_s8();
            ip = static_cast<uint16_t>(ip + rel);
            return true;
        }
        case 0xEC: { // IN AL, DX
            const uint16_t p = dx();
            if (p == 0x03DA) hit_vblank_poll_ = true;
            set_al(static_cast<uint8_t>(port_in_ ? port_in_(p, 1) : 0xFF));
            return true;
        }
        case 0xED: { // IN AX/EAX, DX
            const uint16_t v = port_in_ ? port_in_(dx(), 2) : 0xFFFF;
            if (op32_) eax = v; else set_ax(v);
            return true;
        }
        case 0xEE: // OUT DX, AL
            if (port_out_) port_out_(dx(), al(), 1);
            return true;
        case 0xEF: // OUT DX, AX/EAX
            if (port_out_) port_out_(dx(), ax(), 2);
            return true;

        case 0xF4: // HLT
            halted_ = true;
            return false;
        case 0xF5: // CMC
            set_flag(FLAG_CF, !get_flag(FLAG_CF));
            return true;
        case 0xF6:
        case 0xF7: {
            const ModRM m = decode_modrm(addr32_);
            const int sz = (op == 0xF6) ? 1 : wsize;
            const uint32_t v = read_rm(m, sz);
            switch (m.reg) {
                case 0:
                case 1: { // TEST r/m, imm
                    const uint32_t imm = (sz == 1) ? fetch8() : (sz == 2 ? fetch16() : fetch32());
                    alu_op(4, v, imm, sz);
                    break;
                }
                case 2: // NOT
                    write_rm(m, sz, (~v) & size_mask(sz));
                    break;
                case 3: { // NEG
                    const uint32_t r = alu_op(5, 0, v, sz);
                    write_rm(m, sz, r);
                    break;
                }
                case 4: { // MUL
                    if (sz == 1) {
                        const uint16_t p = static_cast<uint16_t>(al()) * static_cast<uint16_t>(v & 0xFFu);
                        set_ax(p);
                        const bool ov = (p >> 8) != 0;
                        set_flag(FLAG_CF, ov); set_flag(FLAG_OF, ov);
                    } else if (sz == 2) {
                        const uint32_t p = static_cast<uint32_t>(ax()) * static_cast<uint32_t>(v & 0xFFFFu);
                        set_ax(static_cast<uint16_t>(p));
                        set_dx(static_cast<uint16_t>(p >> 16));
                        const bool ov = (p >> 16) != 0;
                        set_flag(FLAG_CF, ov); set_flag(FLAG_OF, ov);
                    } else {
                        const uint64_t p = static_cast<uint64_t>(eax) * static_cast<uint64_t>(v);
                        eax = static_cast<uint32_t>(p);
                        edx = static_cast<uint32_t>(p >> 32);
                        const bool ov = (p >> 32) != 0;
                        set_flag(FLAG_CF, ov); set_flag(FLAG_OF, ov);
                    }
                    break;
                }
                case 5: { // IMUL
                    if (sz == 1) {
                        const int16_t p = static_cast<int8_t>(al()) * static_cast<int8_t>(v & 0xFFu);
                        set_ax(static_cast<uint16_t>(p));
                        const bool ov = (p != static_cast<int8_t>(p & 0xFF));
                        set_flag(FLAG_CF, ov); set_flag(FLAG_OF, ov);
                    } else if (sz == 2) {
                        const int32_t p = static_cast<int16_t>(ax()) * static_cast<int16_t>(v & 0xFFFFu);
                        set_ax(static_cast<uint16_t>(p));
                        set_dx(static_cast<uint16_t>(p >> 16));
                        const bool ov = (p != static_cast<int16_t>(p & 0xFFFF));
                        set_flag(FLAG_CF, ov); set_flag(FLAG_OF, ov);
                    } else {
                        const int64_t p = static_cast<int64_t>(static_cast<int32_t>(eax)) * static_cast<int32_t>(v);
                        eax = static_cast<uint32_t>(p);
                        edx = static_cast<uint32_t>(p >> 32);
                        const bool ov = (p != static_cast<int32_t>(eax));
                        set_flag(FLAG_CF, ov); set_flag(FLAG_OF, ov);
                    }
                    break;
                }
                case 6: { // DIV
                    if (v == 0) { raise_int(0); return true; }
                    if (sz == 1) {
                        const uint16_t q = ax() / (v & 0xFFu);
                        const uint16_t r = ax() % (v & 0xFFu);
                        if (q > 0xFFu) { raise_int(0); return true; }
                        set_al(static_cast<uint8_t>(q));
                        set_ah(static_cast<uint8_t>(r));
                    } else if (sz == 2) {
                        const uint32_t num = (static_cast<uint32_t>(dx()) << 16) | ax();
                        const uint32_t q = num / (v & 0xFFFFu);
                        const uint32_t r = num % (v & 0xFFFFu);
                        if (q > 0xFFFFu) { raise_int(0); return true; }
                        set_ax(static_cast<uint16_t>(q));
                        set_dx(static_cast<uint16_t>(r));
                    } else {
                        const uint64_t num = (static_cast<uint64_t>(edx) << 32) | eax;
                        const uint64_t q = num / v;
                        const uint64_t r = num % v;
                        if (q > 0xFFFFFFFFull) { raise_int(0); return true; }
                        eax = static_cast<uint32_t>(q);
                        edx = static_cast<uint32_t>(r);
                    }
                    break;
                }
                case 7: { // IDIV
                    if (v == 0) { raise_int(0); return true; }
                    if (sz == 1) {
                        const int16_t num = static_cast<int16_t>(ax());
                        const int8_t den = static_cast<int8_t>(v & 0xFFu);
                        const int16_t q = num / den;
                        const int16_t r = num % den;
                        if (q < -128 || q > 127) { raise_int(0); return true; }
                        set_al(static_cast<uint8_t>(q));
                        set_ah(static_cast<uint8_t>(r));
                    } else if (sz == 2) {
                        const int32_t num = static_cast<int32_t>((static_cast<uint32_t>(dx()) << 16) | ax());
                        const int16_t den = static_cast<int16_t>(v & 0xFFFFu);
                        const int32_t q = num / den;
                        const int32_t r = num % den;
                        if (q < -32768 || q > 32767) { raise_int(0); return true; }
                        set_ax(static_cast<uint16_t>(q));
                        set_dx(static_cast<uint16_t>(r));
                    } else {
                        const int64_t num = static_cast<int64_t>((static_cast<uint64_t>(edx) << 32) | eax);
                        const int32_t den = static_cast<int32_t>(v);
                        const int64_t q = num / den;
                        const int64_t r = num % den;
                        if (q < INT32_MIN || q > INT32_MAX) { raise_int(0); return true; }
                        eax = static_cast<uint32_t>(q);
                        edx = static_cast<uint32_t>(r);
                    }
                    break;
                }
            }
            return true;
        }
        case 0xF8: set_flag(FLAG_CF, false); return true;
        case 0xF9: set_flag(FLAG_CF, true); return true;
        case 0xFA: set_flag(FLAG_IF, false); return true;
        case 0xFB: set_flag(FLAG_IF, true); return true;
        case 0xFC: set_flag(FLAG_DF, false); return true;
        case 0xFD: set_flag(FLAG_DF, true); return true;

        case 0xFE: {
            const ModRM m = decode_modrm(addr32_);
            const uint8_t v = static_cast<uint8_t>(read_rm(m, 1));
            if (m.reg == 0) write_rm(m, 1, inc_op(v, 1));
            else if (m.reg == 1) write_rm(m, 1, dec_op(v, 1));
            else unimplemented("0xFE sub-opcode");
            return !error_;
        }
        case 0xFF: {
            const ModRM m = decode_modrm(addr32_);
            switch (m.reg) {
                case 0: write_rm(m, wsize, inc_op(read_rm(m, wsize), wsize)); return true;
                case 1: write_rm(m, wsize, dec_op(read_rm(m, wsize), wsize)); return true;
                case 2: { // CALL near r/m
                    const uint16_t target = static_cast<uint16_t>(read_rm(m, 2));
                    push16(ip);
                    ip = target;
                    return true;
                }
                case 3: { // CALL far m16:16
                    const uint16_t new_ip = seg_read16(m.seg, m.off);
                    const uint16_t new_cs = seg_read16(m.seg, m.off + 2);
                    push16(sreg[SEG_CS]);
                    push16(ip);
                    sreg[SEG_CS] = new_cs;
                    ip = new_ip;
                    return true;
                }
                case 4: // JMP near r/m
                    ip = static_cast<uint16_t>(read_rm(m, 2));
                    return true;
                case 5: { // JMP far m16:16
                    const uint16_t new_ip = seg_read16(m.seg, m.off);
                    const uint16_t new_cs = seg_read16(m.seg, m.off + 2);
                    ip = new_ip;
                    sreg[SEG_CS] = new_cs;
                    return true;
                }
                case 6: { // PUSH r/m
                    const uint32_t v = read_rm(m, wsize);
                    if (op32_) push32(v); else push16(static_cast<uint16_t>(v));
                    return true;
                }
                default:
                    unimplemented("0xFF sub-opcode");
                    return false;
            }
        }
        default:
            unimplemented("primary opcode");
            return false;
    }
}
