#include "sm83.h"
#include "common/bits.h"
#include "logging.h"

SM83::SM83(Bus& bus, Timer& timer)
    : bus(bus), timer(timer) {
}

void SM83::Tick() {
    HandleInterrupts();

    if (halted) {
        timer.AdvanceCycles(4); // 1 cycle
        return;
    }

    if (ime_delay) {
        ime = true;
        ime_delay = false;
    }

    pc_at_opcode = pc;
    const u8 opcode = GetByteFromPC();

    if (!ExecuteOpcode(opcode)) {
        std::exit(0);
    }
}

u8 SM83::GetByteFromPC() {
    u8 byte = bus.Read8(pc++);
    return byte;
}

u16 SM83::GetWordFromPC() {
    u8 low = GetByteFromPC();
    u8 high = GetByteFromPC();

    u16 word = (high << 8) | low;
    return word;
}

void SM83::SetZeroFlag(bool b) {
    if (b) {
        f |= static_cast<u8>(Flags::Zero);
    } else {
        f &= ~static_cast<u8>(Flags::Zero);
    }
}

void SM83::SetNegateFlag(bool b) {
    if (b) {
        f |= static_cast<u8>(Flags::Negate);
    } else {
        f &= ~static_cast<u8>(Flags::Negate);
    }
}

void SM83::SetHalfCarryFlag(bool b) {
    if (b) {
        f |= static_cast<u8>(Flags::HalfCarry);
    } else {
        f &= ~static_cast<u8>(Flags::HalfCarry);
    }
}

void SM83::SetCarryFlag(bool b) {
    if (b) {
        f |= static_cast<u8>(Flags::Carry);
    } else {
        f &= ~static_cast<u8>(Flags::Carry);
    }
}

bool SM83::HasFlag(Flags flag) const {
    return f & static_cast<u8>(flag);
}

template <SM83::Conditions cond>
bool SM83::MeetsCondition() const {
    switch (cond) {
        case Conditions::None:
            return true;
        case Conditions::C:
            return HasFlag(Flags::Carry);
        case Conditions::NC:
            return !HasFlag(Flags::Carry);
        case Conditions::Z:
            return HasFlag(Flags::Zero);
        case Conditions::NZ:
            return !HasFlag(Flags::Zero);
        default:
            UNREACHABLE();
    }
}

template <SM83::Conditions cond>
constexpr std::string_view SM83::GetConditionString() const {
    static_assert(cond != Conditions::None);

    switch (cond) {
        case Conditions::C:
            return "C";
        case Conditions::NC:
            return "NC";
        case Conditions::Z:
            return "Z";
        case Conditions::NZ:
            return "NZ";
        default:
            UNREACHABLE();
    }
}

template <SM83::Registers Register>
constexpr char SM83::Get8bitRegisterName() const {
    static_assert(Register < Registers::AF, "Use Get16bitRegisterName for 16bit registers");

    switch (Register) {
        case Registers::A:
            return 'A';
        case Registers::F:
            return 'F';
        case Registers::B:
            return 'B';
        case Registers::C:
            return 'C';
        case Registers::D:
            return 'D';
        case Registers::E:
            return 'E';
        case Registers::H:
            return 'H';
        case Registers::L:
            return 'L';
        default:
            UNREACHABLE();
    }
}

template <SM83::Registers Register>
u8* SM83::Get8bitRegisterPointer() {
    static_assert(Register < Registers::AF, "Use Get16bitRegisterPointer for 16bit registers");

    switch (Register) {
        case Registers::A:
            return &a;
        case Registers::F:
            return &f;
        case Registers::B:
            return &b;
        case Registers::C:
            return &c;
        case Registers::D:
            return &d;
        case Registers::E:
            return &e;
        case Registers::H:
            return &h;
        case Registers::L:
            return &l;
        default:
            UNREACHABLE();
    }
}

template <SM83::Registers Register>
u8 SM83::Get8bitRegister() const {
    static_assert(Register < Registers::AF, "Use Get16bitRegister for 16bit registers");

    switch (Register) {
        case Registers::A:
            return a;
        case Registers::F:
            return f;
        case Registers::B:
            return b;
        case Registers::C:
            return c;
        case Registers::D:
            return d;
        case Registers::E:
            return e;
        case Registers::H:
            return h;
        case Registers::L:
            return l;
        default:
            UNREACHABLE();
    }
}

template <SM83::Registers Register>
constexpr std::string_view SM83::Get16bitRegisterName() const {
    static_assert(Register >= Registers::AF, "Use Get8bitRegisterName for 8bit registers");

    switch (Register) {
        case Registers::AF:
            return "AF";
        case Registers::BC:
            return "BC";
        case Registers::DE:
            return "DE";
        case Registers::HL:
            return "HL";
        case Registers::SP:
            return "SP";
        default:
            UNREACHABLE();
    }
}

template <SM83::Registers Register>
u16* SM83::Get16bitRegisterPointer() {
    static_assert(Register >= Registers::AF, "Use Get8bitRegisterPointer for 8bit registers");

    switch (Register) {
        case Registers::AF:
            return &af;
        case Registers::BC:
            return &bc;
        case Registers::DE:
            return &de;
        case Registers::HL:
            return &hl;
        case Registers::SP:
            return &sp;
        default:
            UNREACHABLE();
    }
}

template <SM83::Registers Register>
u16 SM83::Get16bitRegister() const {
    static_assert(Register >= Registers::AF, "Use Get8bitRegister for 8bit registers");

    switch (Register) {
        case Registers::AF:
            return af;
        case Registers::BC:
            return bc;
        case Registers::DE:
            return de;
        case Registers::HL:
            return hl;
        case Registers::SP:
            return sp;
        default:
            UNREACHABLE();
    }
}

void SM83::StackPush(u16 word_reg) {
    bus.Write8(--sp, static_cast<u8>((word_reg >> 8) & 0xFF));
    bus.Write8(--sp, static_cast<u8>(word_reg & 0xFF));
}

void SM83::StackPop(u16* word_reg) {
    u8 low = bus.Read8(sp++);
    u8 high = bus.Read8(sp++);

    u16 value = (high << 8) | low;
    *word_reg = value; 
}

bool SM83::ExecuteOpcode(const u8 opcode) {
    switch (opcode) {
#define INSTR(opcode, instr, ...) case opcode: instr; break
        INSTR(0x00, nop());
        INSTR(0x01, ld_bc_d16());
        INSTR(0x02, ld_dbc_a());
        INSTR(0x03, LTRACE("INC BC"); inc_rr(&bc));
        INSTR(0x04, LTRACE("INC B"); inc_r(&b));
        INSTR(0x05, LTRACE("DEC B"); dec_r(&b));
        INSTR(0x06, ld_r_d8<Registers::B>());
        INSTR(0x07, rlca());
        INSTR(0x08, ld_da16_sp());
        INSTR(0x09, add_hl_bc());
        INSTR(0x0A, ld_a_dbc());
        INSTR(0x0B, LTRACE("DEC BC"); dec_rr(&bc));
        INSTR(0x0C, LTRACE("INC C"); inc_r(&c));
        INSTR(0x0D, LTRACE("DEC C"); dec_r(&c));
        INSTR(0x0E, ld_r_d8<Registers::C>());
        INSTR(0x0F, rrca());
        // 0x10 STOP
        INSTR(0x11, ld_de_d16());
        INSTR(0x12, ld_dde_a());
        INSTR(0x13, LTRACE("INC DE"); inc_rr(&de));
        INSTR(0x14, LTRACE("INC D"); inc_r(&d));
        INSTR(0x15, LTRACE("DEC D"); dec_r(&d));
        INSTR(0x16, ld_r_d8<Registers::D>());
        INSTR(0x17, rla());
        INSTR(0x18, jr_r8<Conditions::None>());
        INSTR(0x19, add_hl_de());
        INSTR(0x1A, ld_a_dde());
        INSTR(0x1B, LTRACE("DEC DE"); dec_rr(&de));
        INSTR(0x1C, LTRACE("INC E"); inc_r(&e));
        INSTR(0x1D, LTRACE("DEC E"); dec_r(&e));
        INSTR(0x1E, ld_r_d8<Registers::E>());
        INSTR(0x1F, rra());
        INSTR(0x20, jr_r8<Conditions::NZ>());
        INSTR(0x21, ld_hl_d16());
        INSTR(0x22, ld_dhli_a());
        INSTR(0x23, LTRACE("INC HL"); inc_rr(&hl));
        INSTR(0x24, LTRACE("INC H"); inc_r(&h));
        INSTR(0x25, LTRACE("DEC H"); dec_r(&h));
        INSTR(0x26, ld_r_d8<Registers::H>());
        INSTR(0x27, daa());
        INSTR(0x28, jr_r8<Conditions::Z>());
        INSTR(0x29, add_hl_hl());
        INSTR(0x2A, ld_a_dhli());
        INSTR(0x2B, LTRACE("DEC HL"); dec_rr(&hl));
        INSTR(0x2C, LTRACE("INC L"); inc_r(&l));
        INSTR(0x2D, LTRACE("DEC L"); dec_r(&l));
        INSTR(0x2E, ld_r_d8<Registers::L>());
        INSTR(0x2F, cpl());
        INSTR(0x30, jr_r8<Conditions::NC>());
        INSTR(0x31, ld_sp_d16());
        INSTR(0x32, ld_dhld_a());
        INSTR(0x33, LTRACE("INC SP"); inc_rr(&sp));
        INSTR(0x34, inc_dhl());
        INSTR(0x35, dec_dhl());
        INSTR(0x36, ld_dhl_d8());
        INSTR(0x37, scf());
        INSTR(0x38, jr_r8<Conditions::C>());
        INSTR(0x39, add_hl_sp());
        INSTR(0x3A, ld_a_dhld());
        INSTR(0x3B, LTRACE("DEC SP"); dec_rr(&sp));
        INSTR(0x3C, LTRACE("INC A"); inc_r(&a));
        INSTR(0x3D, LTRACE("DEC A"); dec_r(&a));
        INSTR(0x3E, ld_r_d8<Registers::A>());
        INSTR(0x3F, ccf());
        INSTR(0x40, (ld_r_r<Registers::B, Registers::B>()));
        INSTR(0x41, (ld_r_r<Registers::B, Registers::C>()));
        INSTR(0x42, (ld_r_r<Registers::B, Registers::D>()));
        INSTR(0x43, (ld_r_r<Registers::B, Registers::E>()));
        INSTR(0x44, (ld_r_r<Registers::B, Registers::H>()));
        INSTR(0x45, (ld_r_r<Registers::B, Registers::L>()));
        INSTR(0x46, LTRACE("LD B, (HL)"); ld_r_dhl(&b));
        INSTR(0x47, (ld_r_r<Registers::B, Registers::A>()));
        INSTR(0x48, (ld_r_r<Registers::C, Registers::B>()));
        INSTR(0x49, (ld_r_r<Registers::C, Registers::C>()));
        INSTR(0x4A, (ld_r_r<Registers::C, Registers::D>()));
        INSTR(0x4B, (ld_r_r<Registers::C, Registers::E>()));
        INSTR(0x4C, (ld_r_r<Registers::C, Registers::H>()));
        INSTR(0x4D, (ld_r_r<Registers::C, Registers::L>()));
        INSTR(0x4E, LTRACE("LD C, (HL)"); ld_r_dhl(&c));
        INSTR(0x4F, (ld_r_r<Registers::C, Registers::A>()));
        INSTR(0x50, (ld_r_r<Registers::D, Registers::B>()));
        INSTR(0x51, (ld_r_r<Registers::D, Registers::C>()));
        INSTR(0x52, (ld_r_r<Registers::D, Registers::D>()));
        INSTR(0x53, (ld_r_r<Registers::D, Registers::E>()));
        INSTR(0x54, (ld_r_r<Registers::D, Registers::H>()));
        INSTR(0x55, (ld_r_r<Registers::D, Registers::L>()));
        INSTR(0x56, LTRACE("LD D, (HL)"); ld_r_dhl(&d));
        INSTR(0x57, (ld_r_r<Registers::D, Registers::A>()));
        INSTR(0x58, (ld_r_r<Registers::E, Registers::B>()));
        INSTR(0x59, (ld_r_r<Registers::E, Registers::C>()));
        INSTR(0x5A, (ld_r_r<Registers::E, Registers::D>()));
        INSTR(0x5B, (ld_r_r<Registers::E, Registers::E>()));
        INSTR(0x5C, (ld_r_r<Registers::E, Registers::H>()));
        INSTR(0x5D, (ld_r_r<Registers::E, Registers::L>()));
        INSTR(0x5E, LTRACE("LD E, (HL)"); ld_r_dhl(&e));
        INSTR(0x5F, (ld_r_r<Registers::E, Registers::A>()));
        INSTR(0x60, (ld_r_r<Registers::H, Registers::B>()));
        INSTR(0x61, (ld_r_r<Registers::H, Registers::C>()));
        INSTR(0x62, (ld_r_r<Registers::H, Registers::D>()));
        INSTR(0x63, (ld_r_r<Registers::H, Registers::E>()));
        INSTR(0x64, (ld_r_r<Registers::H, Registers::H>()));
        INSTR(0x65, (ld_r_r<Registers::H, Registers::L>()));
        INSTR(0x66, LTRACE("LD H, (HL)"); ld_r_dhl(&h));
        INSTR(0x67, (ld_r_r<Registers::H, Registers::A>()));
        INSTR(0x68, (ld_r_r<Registers::L, Registers::B>()));
        INSTR(0x69, (ld_r_r<Registers::L, Registers::C>()));
        INSTR(0x6A, (ld_r_r<Registers::L, Registers::D>()));
        INSTR(0x6B, (ld_r_r<Registers::L, Registers::E>()));
        INSTR(0x6C, (ld_r_r<Registers::L, Registers::H>()));
        INSTR(0x6D, (ld_r_r<Registers::L, Registers::L>()));
        INSTR(0x6E, LTRACE("LD L, (HL)"); ld_r_dhl(&l));
        INSTR(0x6F, (ld_r_r<Registers::L, Registers::A>()));
        INSTR(0x70, LTRACE("LD (HL), B"); ld_dhl_r(b));
        INSTR(0x71, LTRACE("LD (HL), C"); ld_dhl_r(c));
        INSTR(0x72, LTRACE("LD (HL), D"); ld_dhl_r(d));
        INSTR(0x73, LTRACE("LD (HL), E"); ld_dhl_r(e));
        INSTR(0x74, LTRACE("LD (HL), H"); ld_dhl_r(h));
        INSTR(0x75, LTRACE("LD (HL), L"); ld_dhl_r(l));
        INSTR(0x76, halt());
        INSTR(0x77, LTRACE("LD (HL), A"); ld_dhl_r(a));
        INSTR(0x78, (ld_r_r<Registers::A, Registers::B>()));
        INSTR(0x79, (ld_r_r<Registers::A, Registers::C>()));
        INSTR(0x7A, (ld_r_r<Registers::A, Registers::D>()));
        INSTR(0x7B, (ld_r_r<Registers::A, Registers::E>()));
        INSTR(0x7C, (ld_r_r<Registers::A, Registers::H>()));
        INSTR(0x7D, (ld_r_r<Registers::A, Registers::L>()));
        INSTR(0x7E, LTRACE("LD A, (HL)"); ld_r_dhl(&a));
        INSTR(0x7F, (ld_r_r<Registers::A, Registers::A>()));
        INSTR(0x80, LTRACE("ADD A, B"); add_a_r(b));
        INSTR(0x81, LTRACE("ADD A, C"); add_a_r(c));
        INSTR(0x82, LTRACE("ADD A, D"); add_a_r(d));
        INSTR(0x83, LTRACE("ADD A, E"); add_a_r(e));
        INSTR(0x84, LTRACE("ADD A, H"); add_a_r(h));
        INSTR(0x85, LTRACE("ADD A, L"); add_a_r(l));
        INSTR(0x86, add_a_dhl());
        INSTR(0x87, LTRACE("ADD A, A"); add_a_r(a));
        INSTR(0x88, LTRACE("ADC A, B"); adc_a_r(b));
        INSTR(0x89, LTRACE("ADC A, C"); adc_a_r(c));
        INSTR(0x8A, LTRACE("ADC A, D"); adc_a_r(d));
        INSTR(0x8B, LTRACE("ADC A, E"); adc_a_r(e));
        INSTR(0x8C, LTRACE("ADC A, H"); adc_a_r(h));
        INSTR(0x8D, LTRACE("ADC A, L"); adc_a_r(l));
        INSTR(0x8E, LTRACE("ADC A, (HL)"); adc_a_r(bus.Read8(hl)));
        INSTR(0x8F, LTRACE("ADC A, A"); adc_a_r(a));
        INSTR(0x90, LTRACE("SUB B"); sub_r(b));
        INSTR(0x91, LTRACE("SUB C"); sub_r(c));
        INSTR(0x92, LTRACE("SUB D"); sub_r(d));
        INSTR(0x93, LTRACE("SUB E"); sub_r(e));
        INSTR(0x94, LTRACE("SUB H"); sub_r(h));
        INSTR(0x95, LTRACE("SUB L"); sub_r(l));
        INSTR(0x96, LTRACE("SUB (HL)"); sub_r(bus.Read8(hl)));
        INSTR(0x97, LTRACE("SUB A"); sub_r(a));
        INSTR(0x98, LTRACE("SBC B"); sbc_r(b));
        INSTR(0x99, LTRACE("SBC C"); sbc_r(c));
        INSTR(0x9A, LTRACE("SBC D"); sbc_r(d));
        INSTR(0x9B, LTRACE("SBC E"); sbc_r(e));
        INSTR(0x9C, LTRACE("SBC H"); sbc_r(h));
        INSTR(0x9D, LTRACE("SBC L"); sbc_r(l));
        INSTR(0x9E, sbc_dhl());
        INSTR(0x9F, LTRACE("SBC A"); sbc_r(a));
        INSTR(0xA0, LTRACE("AND B"); and_r(b));
        INSTR(0xA1, LTRACE("AND C"); and_r(c));
        INSTR(0xA2, LTRACE("AND D"); and_r(d));
        INSTR(0xA3, LTRACE("AND E"); and_r(e));
        INSTR(0xA4, LTRACE("AND H"); and_r(h));
        INSTR(0xA5, LTRACE("AND L"); and_r(l));
        INSTR(0xA6, LTRACE("AND (HL)"); and_r(bus.Read8(hl)));
        INSTR(0xA7, LTRACE("AND A"); and_r(a));
        INSTR(0xA8, LTRACE("XOR B"); xor_r(b));
        INSTR(0xA9, LTRACE("XOR C"); xor_r(c));
        INSTR(0xAA, LTRACE("XOR D"); xor_r(d));
        INSTR(0xAB, LTRACE("XOR E"); xor_r(e));
        INSTR(0xAC, LTRACE("XOR H"); xor_r(h));
        INSTR(0xAD, LTRACE("XOR L"); xor_r(l));
        INSTR(0xAE, xor_dhl());
        INSTR(0xAF, LTRACE("XOR A"); xor_r(a));
        INSTR(0xB0, LTRACE("OR B"); or_r(b));
        INSTR(0xB1, LTRACE("OR C"); or_r(c));
        INSTR(0xB2, LTRACE("OR D"); or_r(d));
        INSTR(0xB3, LTRACE("OR E"); or_r(e));
        INSTR(0xB4, LTRACE("OR H"); or_r(h));
        INSTR(0xB5, LTRACE("OR L"); or_r(l));
        INSTR(0xB6, or_dhl());
        INSTR(0xB7, LTRACE("OR A"); or_r(a));
        INSTR(0xB8, LTRACE("CP B"); cp_r(b));
        INSTR(0xB9, LTRACE("CP C"); cp_r(c));
        INSTR(0xBA, LTRACE("CP D"); cp_r(d));
        INSTR(0xBB, LTRACE("CP E"); cp_r(e));
        INSTR(0xBC, LTRACE("CP H"); cp_r(h));
        INSTR(0xBD, LTRACE("CP L"); cp_r(l));
        INSTR(0xBE, cp_dhl());
        INSTR(0xBF, LTRACE("CP A"); cp_r(a));
        INSTR(0xC0, ret<Conditions::NZ>());
        INSTR(0xC1, pop_rr<Registers::BC>());
        INSTR(0xC2, jp_a16<Conditions::NZ>());
        INSTR(0xC3, jp_a16<Conditions::None>());
        INSTR(0xC4, call_a16<Conditions::NZ>());
        INSTR(0xC5, push_rr<Registers::BC>());
        INSTR(0xC6, add_a_d8());
        INSTR(0xC7, rst(0x00));
        INSTR(0xC8, ret<Conditions::Z>());
        INSTR(0xC9, ret<Conditions::None>());
        INSTR(0xCA, jp_a16<Conditions::Z>());
        case  0xCB: return ExecuteCBOpcode(GetByteFromPC());
        INSTR(0xCC, call_a16<Conditions::Z>());
        INSTR(0xCD, call_a16<Conditions::None>());
        INSTR(0xCE, adc_a_d8());
        INSTR(0xCF, rst(0x08));
        INSTR(0xD0, ret<Conditions::NC>());
        INSTR(0xD1, pop_rr<Registers::DE>());
        INSTR(0xD2, jp_a16<Conditions::NC>());
        INSTR(0xD3, ill(opcode); return false);
        INSTR(0xD4, call_a16<Conditions::NC>());
        INSTR(0xD5, push_rr<Registers::DE>());
        INSTR(0xD6, sub_d8());
        INSTR(0xD7, rst(0x10));
        INSTR(0xD8, ret<Conditions::C>());
        INSTR(0xD9, reti());
        INSTR(0xDA, jp_a16<Conditions::C>());
        INSTR(0xDB, ill(opcode); return false);
        INSTR(0xDC, call_a16<Conditions::C>());
        INSTR(0xDD, ill(opcode); return false);
        INSTR(0xDE, sbc_a_d8());
        INSTR(0xDF, rst(0x18));
        INSTR(0xE0, ldh_da8_a());
        INSTR(0xE1, pop_rr<Registers::HL>());
        INSTR(0xE2, ld_dc_a());
        INSTR(0xE3, ill(opcode); return false);
        INSTR(0xE4, ill(opcode); return false);
        INSTR(0xE5, push_rr<Registers::HL>());
        INSTR(0xE6, and_d8());
        INSTR(0xE7, rst(0x20));
        INSTR(0xE8, add_sp_d8());
        INSTR(0xE9, jp_hl());
        INSTR(0xEA, ld_da16_a());
        INSTR(0xEB, ill(opcode); return false);
        INSTR(0xEC, ill(opcode); return false);
        INSTR(0xED, ill(opcode); return false);
        INSTR(0xEE, xor_d8());
        INSTR(0xEF, rst(0x28));
        INSTR(0xF0, ldh_a_da8());
        INSTR(0xF1, pop_rr<Registers::AF>());
        INSTR(0xF2, ld_a_dc());
        INSTR(0xF3, di());
        INSTR(0xF4, ill(opcode); return false);
        INSTR(0xF5, push_rr<Registers::AF>());
        INSTR(0xF6, or_d8());
        INSTR(0xF7, rst(0x30));
        INSTR(0xF8, ld_hl_sp_d8());
        INSTR(0xF9, ld_sp_hl());
        INSTR(0xFA, ld_a_da16());
        INSTR(0xFB, ei());
        INSTR(0xFC, ill(opcode); return false);
        INSTR(0xFD, ill(opcode); return false);
        INSTR(0xFE, cp_d8());
        INSTR(0xFF, rst(0x38));

        default:
            bus.DumpMemoryToFile();
            LFATAL("unimplemented opcode 0x{:02X} at 0x{:04X}", opcode, pc_at_opcode);
            DumpRegisters();
            return false;
    }

    return true;
}

bool SM83::ExecuteCBOpcode(const u8 opcode) {
    switch (opcode) {
        INSTR(0x00, LTRACE("RLC B"); rlc_r(&b));
        INSTR(0x01, LTRACE("RLC C"); rlc_r(&c));
        INSTR(0x02, LTRACE("RLC D"); rlc_r(&d));
        INSTR(0x03, LTRACE("RLC E"); rlc_r(&e));
        INSTR(0x04, LTRACE("RLC H"); rlc_r(&h));
        INSTR(0x05, LTRACE("RLC L"); rlc_r(&l));
        INSTR(0x06, rlc_dhl());
        INSTR(0x07, LTRACE("RLC A"); rlc_r(&a));
        INSTR(0x08, LTRACE("RRC B"); rrc_r(&b));
        INSTR(0x09, LTRACE("RRC C"); rrc_r(&c));
        INSTR(0x0A, LTRACE("RRC D"); rrc_r(&d));
        INSTR(0x0B, LTRACE("RRC E"); rrc_r(&e));
        INSTR(0x0C, LTRACE("RRC H"); rrc_r(&h));
        INSTR(0x0D, LTRACE("RRC L"); rrc_r(&l));
        INSTR(0x0E, rrc_dhl());
        INSTR(0x0F, LTRACE("RRC A"); rrc_r(&a));
        INSTR(0x10, LTRACE("RL B"); rl_r(&b));
        INSTR(0x11, LTRACE("RL C"); rl_r(&c));
        INSTR(0x12, LTRACE("RL D"); rl_r(&d));
        INSTR(0x13, LTRACE("RL E"); rl_r(&e));
        INSTR(0x14, LTRACE("RL H"); rl_r(&h));
        INSTR(0x15, LTRACE("RL L"); rl_r(&l));
        INSTR(0x16, rl_dhl());
        INSTR(0x17, LTRACE("RL A"); rl_r(&a));
        INSTR(0x18, LTRACE("RR B"); rr_r(&b));
        INSTR(0x19, LTRACE("RR C"); rr_r(&c));
        INSTR(0x1A, LTRACE("RR D"); rr_r(&d));
        INSTR(0x1B, LTRACE("RR E"); rr_r(&e));
        INSTR(0x1C, LTRACE("RR H"); rr_r(&h));
        INSTR(0x1D, LTRACE("RR L"); rr_r(&l));
        INSTR(0x1E, rr_dhl());
        INSTR(0x1F, LTRACE("RR A"); rr_r(&a));
        INSTR(0x20, LTRACE("SLA B"); sla_r(&b));
        INSTR(0x21, LTRACE("SLA C"); sla_r(&c));
        INSTR(0x22, LTRACE("SLA D"); sla_r(&d));
        INSTR(0x23, LTRACE("SLA E"); sla_r(&e));
        INSTR(0x24, LTRACE("SLA H"); sla_r(&h));
        INSTR(0x25, LTRACE("SLA L"); sla_r(&l));
        INSTR(0x26, sla_dhl());
        INSTR(0x27, LTRACE("SLA A"); sla_r(&a));
        INSTR(0x28, LTRACE("SRA B"); sra_r(&b));
        INSTR(0x29, LTRACE("SRA C"); sra_r(&c));
        INSTR(0x2A, LTRACE("SRA D"); sra_r(&d));
        INSTR(0x2B, LTRACE("SRA E"); sra_r(&e));
        INSTR(0x2C, LTRACE("SRA H"); sra_r(&h));
        INSTR(0x2D, LTRACE("SRA L"); sra_r(&l));
        INSTR(0x2E, sra_dhl());
        INSTR(0x2F, LTRACE("SRA A"); sra_r(&a));
        INSTR(0x30, LTRACE("SWAP B"); swap_r(&b));
        INSTR(0x31, LTRACE("SWAP C"); swap_r(&c));
        INSTR(0x32, LTRACE("SWAP D"); swap_r(&d));
        INSTR(0x33, LTRACE("SWAP E"); swap_r(&e));
        INSTR(0x34, LTRACE("SWAP H"); swap_r(&h));
        INSTR(0x35, LTRACE("SWAP L"); swap_r(&l));
        INSTR(0x36, swap_dhl());
        INSTR(0x37, LTRACE("SWAP A"); swap_r(&a));
        INSTR(0x38, LTRACE("SRL B"); srl_r(&b));
        INSTR(0x39, LTRACE("SRL C"); srl_r(&c));
        INSTR(0x3A, LTRACE("SRL D"); srl_r(&d));
        INSTR(0x3B, LTRACE("SRL E"); srl_r(&e));
        INSTR(0x3C, LTRACE("SRL H"); srl_r(&h));
        INSTR(0x3D, LTRACE("SRL L"); srl_r(&l));
        INSTR(0x3E, srl_dhl());
        INSTR(0x3F, LTRACE("SRL A"); srl_r(&a));
        INSTR(0x40, (bit<0, Registers::B>()));
        INSTR(0x41, (bit<0, Registers::C>()));
        INSTR(0x42, (bit<0, Registers::D>()));
        INSTR(0x43, (bit<0, Registers::E>()));
        INSTR(0x44, (bit<0, Registers::H>()));
        INSTR(0x45, (bit<0, Registers::L>()));
        INSTR(0x46, bit_dhl<0>());
        INSTR(0x47, (bit<0, Registers::A>()));
        INSTR(0x48, (bit<1, Registers::B>()));
        INSTR(0x49, (bit<1, Registers::C>()));
        INSTR(0x4A, (bit<1, Registers::D>()));
        INSTR(0x4B, (bit<1, Registers::E>()));
        INSTR(0x4C, (bit<1, Registers::H>()));
        INSTR(0x4D, (bit<1, Registers::L>()));
        INSTR(0x4E, bit_dhl<1>());
        INSTR(0x4F, (bit<1, Registers::A>()));
        INSTR(0x50, (bit<2, Registers::B>()));
        INSTR(0x51, (bit<2, Registers::C>()));
        INSTR(0x52, (bit<2, Registers::D>()));
        INSTR(0x53, (bit<2, Registers::E>()));
        INSTR(0x54, (bit<2, Registers::H>()));
        INSTR(0x55, (bit<2, Registers::L>()));
        INSTR(0x56, bit_dhl<2>());
        INSTR(0x57, (bit<2, Registers::A>()));
        INSTR(0x58, (bit<3, Registers::B>()));
        INSTR(0x59, (bit<3, Registers::C>()));
        INSTR(0x5A, (bit<3, Registers::D>()));
        INSTR(0x5B, (bit<3, Registers::E>()));
        INSTR(0x5C, (bit<3, Registers::H>()));
        INSTR(0x5D, (bit<3, Registers::L>()));
        INSTR(0x5E, bit_dhl<3>());
        INSTR(0x5F, (bit<3, Registers::A>()));
        INSTR(0x60, (bit<4, Registers::B>()));
        INSTR(0x61, (bit<4, Registers::C>()));
        INSTR(0x62, (bit<4, Registers::D>()));
        INSTR(0x63, (bit<4, Registers::E>()));
        INSTR(0x64, (bit<4, Registers::H>()));
        INSTR(0x65, (bit<4, Registers::L>()));
        INSTR(0x66, bit_dhl<4>());
        INSTR(0x67, (bit<4, Registers::A>()));
        INSTR(0x68, (bit<5, Registers::B>()));
        INSTR(0x69, (bit<5, Registers::C>()));
        INSTR(0x6A, (bit<5, Registers::D>()));
        INSTR(0x6B, (bit<5, Registers::E>()));
        INSTR(0x6C, (bit<5, Registers::H>()));
        INSTR(0x6D, (bit<5, Registers::L>()));
        INSTR(0x6E, bit_dhl<5>());
        INSTR(0x6F, (bit<5, Registers::A>()));
        INSTR(0x70, (bit<6, Registers::B>()));
        INSTR(0x71, (bit<6, Registers::C>()));
        INSTR(0x72, (bit<6, Registers::D>()));
        INSTR(0x73, (bit<6, Registers::E>()));
        INSTR(0x74, (bit<6, Registers::H>()));
        INSTR(0x75, (bit<6, Registers::L>()));
        INSTR(0x76, bit_dhl<6>());
        INSTR(0x77, (bit<6, Registers::A>()));
        INSTR(0x78, (bit<7, Registers::B>()));
        INSTR(0x79, (bit<7, Registers::C>()));
        INSTR(0x7A, (bit<7, Registers::D>()));
        INSTR(0x7B, (bit<7, Registers::E>()));
        INSTR(0x7C, (bit<7, Registers::H>()));
        INSTR(0x7D, (bit<7, Registers::L>()));
        INSTR(0x7E, bit_dhl<7>());
        INSTR(0x7F, (bit<7, Registers::A>()));
        INSTR(0x80, (res<0, Registers::B>()));
        INSTR(0x81, (res<0, Registers::C>()));
        INSTR(0x82, (res<0, Registers::D>()));
        INSTR(0x83, (res<0, Registers::E>()));
        INSTR(0x84, (res<0, Registers::H>()));
        INSTR(0x85, (res<0, Registers::L>()));
        INSTR(0x86, res_dhl(0));
        INSTR(0x87, (res<0, Registers::A>()));
        INSTR(0x88, (res<1, Registers::B>()));
        INSTR(0x89, (res<1, Registers::C>()));
        INSTR(0x8A, (res<1, Registers::D>()));
        INSTR(0x8B, (res<1, Registers::E>()));
        INSTR(0x8C, (res<1, Registers::H>()));
        INSTR(0x8D, (res<1, Registers::L>()));
        INSTR(0x8E, res_dhl(1));
        INSTR(0x8F, (res<1, Registers::A>()));
        INSTR(0x90, (res<2, Registers::B>()));
        INSTR(0x91, (res<2, Registers::C>()));
        INSTR(0x92, (res<2, Registers::D>()));
        INSTR(0x93, (res<2, Registers::E>()));
        INSTR(0x94, (res<2, Registers::H>()));
        INSTR(0x95, (res<2, Registers::L>()));
        INSTR(0x96, res_dhl(2));
        INSTR(0x97, (res<2, Registers::A>()));
        INSTR(0x98, (res<3, Registers::B>()));
        INSTR(0x99, (res<3, Registers::C>()));
        INSTR(0x9A, (res<3, Registers::D>()));
        INSTR(0x9B, (res<3, Registers::E>()));
        INSTR(0x9C, (res<3, Registers::H>()));
        INSTR(0x9D, (res<3, Registers::L>()));
        INSTR(0x9E, res_dhl(3));
        INSTR(0x9F, (res<3, Registers::A>()));
        INSTR(0xA0, (res<4, Registers::B>()));
        INSTR(0xA1, (res<4, Registers::C>()));
        INSTR(0xA2, (res<4, Registers::D>()));
        INSTR(0xA3, (res<4, Registers::E>()));
        INSTR(0xA4, (res<4, Registers::H>()));
        INSTR(0xA5, (res<4, Registers::L>()));
        INSTR(0xA6, res_dhl(4));
        INSTR(0xA7, (res<4, Registers::A>()));
        INSTR(0xA8, (res<5, Registers::B>()));
        INSTR(0xA9, (res<5, Registers::C>()));
        INSTR(0xAA, (res<5, Registers::D>()));
        INSTR(0xAB, (res<5, Registers::E>()));
        INSTR(0xAC, (res<5, Registers::H>()));
        INSTR(0xAD, (res<5, Registers::L>()));
        INSTR(0xAE, res_dhl(5));
        INSTR(0xAF, (res<5, Registers::A>()));
        INSTR(0xB0, (res<6, Registers::B>()));
        INSTR(0xB1, (res<6, Registers::C>()));
        INSTR(0xB2, (res<6, Registers::D>()));
        INSTR(0xB3, (res<6, Registers::E>()));
        INSTR(0xB4, (res<6, Registers::H>()));
        INSTR(0xB5, (res<6, Registers::L>()));
        INSTR(0xB6, res_dhl(6));
        INSTR(0xB7, (res<6, Registers::A>()));
        INSTR(0xB8, (res<7, Registers::B>()));
        INSTR(0xB9, (res<7, Registers::C>()));
        INSTR(0xBA, (res<7, Registers::D>()));
        INSTR(0xBB, (res<7, Registers::E>()));
        INSTR(0xBC, (res<7, Registers::H>()));
        INSTR(0xBD, (res<7, Registers::L>()));
        INSTR(0xBE, res_dhl(7));
        INSTR(0xBF, (res<7, Registers::A>()));
        INSTR(0xC0, (set<0, Registers::B>()));
        INSTR(0xC1, (set<0, Registers::C>()));
        INSTR(0xC2, (set<0, Registers::D>()));
        INSTR(0xC3, (set<0, Registers::E>()));
        INSTR(0xC4, (set<0, Registers::H>()));
        INSTR(0xC5, (set<0, Registers::L>()));
        INSTR(0xC6, set_dhl<0>());
        INSTR(0xC7, (set<0, Registers::A>()));
        INSTR(0xC8, (set<1, Registers::B>()));
        INSTR(0xC9, (set<1, Registers::C>()));
        INSTR(0xCA, (set<1, Registers::D>()));
        INSTR(0xCB, (set<1, Registers::E>()));
        INSTR(0xCC, (set<1, Registers::H>()));
        INSTR(0xCD, (set<1, Registers::L>()));
        INSTR(0xCE, set_dhl<1>());
        INSTR(0xCF, (set<1, Registers::A>()));
        INSTR(0xD0, (set<2, Registers::B>()));
        INSTR(0xD1, (set<2, Registers::C>()));
        INSTR(0xD2, (set<2, Registers::D>()));
        INSTR(0xD3, (set<2, Registers::E>()));
        INSTR(0xD4, (set<2, Registers::H>()));
        INSTR(0xD5, (set<2, Registers::L>()));
        INSTR(0xD6, set_dhl<2>());
        INSTR(0xD7, (set<2, Registers::A>()));
        INSTR(0xD8, (set<3, Registers::B>()));
        INSTR(0xD9, (set<3, Registers::C>()));
        INSTR(0xDA, (set<3, Registers::D>()));
        INSTR(0xDB, (set<3, Registers::E>()));
        INSTR(0xDC, (set<3, Registers::H>()));
        INSTR(0xDD, (set<3, Registers::L>()));
        INSTR(0xDE, set_dhl<3>());
        INSTR(0xDF, (set<3, Registers::A>()));
        INSTR(0xE0, (set<4, Registers::B>()));
        INSTR(0xE1, (set<4, Registers::C>()));
        INSTR(0xE2, (set<4, Registers::D>()));
        INSTR(0xE3, (set<4, Registers::E>()));
        INSTR(0xE4, (set<4, Registers::H>()));
        INSTR(0xE5, (set<4, Registers::L>()));
        INSTR(0xE6, set_dhl<4>());
        INSTR(0xE7, (set<4, Registers::A>()));
        INSTR(0xE8, (set<5, Registers::B>()));
        INSTR(0xE9, (set<5, Registers::C>()));
        INSTR(0xEA, (set<5, Registers::D>()));
        INSTR(0xEB, (set<5, Registers::E>()));
        INSTR(0xEC, (set<5, Registers::H>()));
        INSTR(0xED, (set<5, Registers::L>()));
        INSTR(0xEE, set_dhl<5>());
        INSTR(0xEF, (set<5, Registers::A>()));
        INSTR(0xF0, (set<6, Registers::B>()));
        INSTR(0xF1, (set<6, Registers::C>()));
        INSTR(0xF2, (set<6, Registers::D>()));
        INSTR(0xF3, (set<6, Registers::E>()));
        INSTR(0xF4, (set<6, Registers::H>()));
        INSTR(0xF5, (set<6, Registers::L>()));
        INSTR(0xF6, set_dhl<6>());
        INSTR(0xF7, (set<6, Registers::A>()));
        INSTR(0xF8, (set<7, Registers::B>()));
        INSTR(0xF9, (set<7, Registers::C>()));
        INSTR(0xFA, (set<7, Registers::D>()));
        INSTR(0xFB, (set<7, Registers::E>()));
        INSTR(0xFC, (set<7, Registers::H>()));
        INSTR(0xFD, (set<7, Registers::L>()));
        INSTR(0xFE, set_dhl<7>());
        INSTR(0xFF, (set<7, Registers::A>()));
#undef INSTR
        default:
            UNREACHABLE();
    }

    return true;
}

void SM83::HandleInterrupts() {
    u8 interrupt_flags = bus.Read8(0xFF0F, false);
    u8 interrupt_enable = bus.Read8(0xFFFF, false);
    u8 potential_interrupts = interrupt_flags & interrupt_enable & 0x1F;
    if (!potential_interrupts) {
        return;
    }

    // there are 5 different kinds of interrupts
    for (u8 i = 0; i < 5; i++) {
        u8 flag = 1 << i;
        if (!(potential_interrupts & flag)) {
            continue;
        }

        if (!ime) {
            if (halted) {
                halted = false;
            }

            return;
        }

        bus.Write8(0xFF0F, interrupt_flags & ~flag, false);
        ime = false;

        uint16_t address = 0x0000;

#define ADDR(bit, addr) if (i == bit) address = static_cast<u16>(InterruptAddresses::addr)
        ADDR(0, VBlank);
        ADDR(1, LCDCStatus);
        ADDR(2, Timer);
        ADDR(3, Serial);
        ADDR(4, Joypad);
#undef ADDR

        StackPush(pc);
        pc = address;

        halted = false;
    }
}

void SM83::DumpRegisters() {
    LFATAL("AF={:04X} BC={:04X} DE={:04X} HL={:04X} SP={:04X} PC={:04X}", af, bc, de, hl, sp, pc_at_opcode);
    if (!f) {
        LFATAL("Flags: none");
    } else {
        LFATAL("Flags: [{}{}{}{}]", (HasFlag(Flags::Zero)) ? 'Z' : ' ',
                                    (HasFlag(Flags::Negate)) ? 'N' : ' ',
                                    (HasFlag(Flags::HalfCarry)) ? 'H' : ' ',
                                    (HasFlag(Flags::Carry)) ? 'C' : ' ');
    }
}

void SM83::ill(const u8 opcode) {
    bus.DumpMemoryToFile();
    LFATAL("illegal opcode 0x{:02X} at 0x{:04X}", opcode, pc_at_opcode);
    DumpRegisters();
}

void SM83::adc_a_d8() {
    u8 value = GetByteFromPC();
    LTRACE("ADC A, 0x{:02X}", value);

    bool carry = HasFlag(Flags::Carry);
    u16 full = a + value + carry;
    u8 result = static_cast<u8>(full);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(((a & 0xF) + (value & 0xF) + carry) > 0xF);
    SetCarryFlag(full > 0xFF);

    a = result;
}

void SM83::adc_a_r(u8 reg) {
    bool carry = HasFlag(Flags::Carry);
    u16 full = a + reg + carry;
    u8 result = static_cast<u8>(full);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(((a & 0xF) + (reg & 0xF) + carry) > 0xF);
    SetCarryFlag(full > 0xFF);

    a = result;
}

void SM83::add_a_d8() {
    u8 value = GetByteFromPC();
    LTRACE("ADD A, 0x{:02X}", value);
    u16 result = a + value;

    SetZeroFlag(static_cast<u8>(result) == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag((a & 0xF) + (value & 0xF) > 0xF);
    SetCarryFlag(result > 0xFF);

    a = static_cast<u8>(result);
}

void SM83::add_a_dhl() {
    LTRACE("ADD A, (HL)");
    u8 value = bus.Read8(hl);
    u16 result = a + value;

    SetZeroFlag(static_cast<u8>(result) == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag((a & 0xF) + (value & 0xF) > 0xF);
    SetCarryFlag(result > 0xFF);

    a = static_cast<u8>(result);
}

void SM83::add_a_r(u8 reg) {
    u16 result = a + reg;

    SetZeroFlag(static_cast<u8>(result) == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag((a & 0xF) + (reg & 0xF) > 0xF);
    SetCarryFlag(result > 0xFF);

    a = static_cast<u8>(result);
}

void SM83::add_hl_bc() {
    LTRACE("ADD HL, BC");
    u32 result = hl + bc;

    SetNegateFlag(false);
    SetHalfCarryFlag(((hl & 0xFFF) + (bc & 0xFFF)) & 0x1000);
    SetCarryFlag((result & 0x10000) != 0);

    hl = static_cast<u16>(result);

    timer.AdvanceCycles(4);
}

void SM83::add_hl_de() {
    LTRACE("ADD HL, DE");
    u32 result = hl + de;

    SetNegateFlag(false);
    SetHalfCarryFlag(((hl & 0xFFF) + (de & 0xFFF)) & 0x1000);
    SetCarryFlag((result & 0x10000) != 0);

    hl = static_cast<u16>(result);

    timer.AdvanceCycles(4);
}

void SM83::add_hl_hl() {
    LTRACE("ADD HL, HL");
    u32 result = hl + hl;

    SetNegateFlag(false);
    SetHalfCarryFlag(((hl & 0xFFF) + (hl & 0xFFF)) & 0x1000);
    SetCarryFlag((result & 0x10000) != 0);

    hl = static_cast<u16>(result);

    timer.AdvanceCycles(4);
}

void SM83::add_hl_sp() {
    LTRACE("ADD HL, SP");
    u32 result = hl + sp;

    SetNegateFlag(false);
    SetHalfCarryFlag(((hl & 0xFFF) + (sp & 0xFFF)) & 0x1000);
    SetCarryFlag((result & 0x10000) != 0);

    hl = static_cast<u16>(result);

    timer.AdvanceCycles(4);
}

void SM83::add_sp_d8() {
    s8 value = static_cast<s8>(GetByteFromPC());
    LTRACE("ADD SP, 0x{:02X}", value);
    u32 result = sp + value;

    SetZeroFlag(false);
    SetNegateFlag(false);
    SetHalfCarryFlag(((sp ^ value ^ (result & 0xFFFF)) & 0x10) == 0x10);
    SetCarryFlag(((sp ^ value ^ (result & 0xFFFF)) & 0x100) == 0x100);

    sp = static_cast<u16>(result);

    timer.AdvanceCycles(8);
}

void SM83::and_d8() {
    u8 value = GetByteFromPC();
    LTRACE("AND 0x{:02X}", value);

    a &= value;

    SetZeroFlag(a == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(true);
    SetCarryFlag(false);
}

void SM83::and_r(u8 reg) {
    a &= reg;

    SetZeroFlag(a == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(true);
    SetCarryFlag(false);
}

template <u8 Bit, SM83::Registers Register>
void SM83::bit() {
    LTRACE("BIT {}, {}", Bit, Get8bitRegisterName<Register>());
    const u8 reg = Get8bitRegister<Register>();

    SetZeroFlag(!Common::IsBitSet<Bit>(reg));
    SetNegateFlag(false);
    SetHalfCarryFlag(true);
}

template <u8 Bit>
void SM83::bit_dhl() {
    LTRACE("BIT {}, (HL)", Bit);
    const u8 value = bus.Read8(hl);

    SetZeroFlag(!Common::IsBitSet<Bit>(value));
    SetNegateFlag(false);
    SetHalfCarryFlag(true);
}

template <SM83::Conditions cond>
void SM83::call_a16() {
    const u16 address = GetWordFromPC();

    if constexpr (cond == Conditions::None) {
        LTRACE("CALL 0x{:04X}", address);
    } else {
        LTRACE("CALL {}, 0x{:04X}", GetConditionString<cond>(), address);
    }

    if (MeetsCondition<cond>()) {
        StackPush(pc);
        pc = address;
        timer.AdvanceCycles(4);
    }
}

void SM83::ccf() {
    LTRACE("CCF");

    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(!HasFlag(Flags::Carry));
}

void SM83::cp_d8() {
    u8 value = GetByteFromPC();
    LTRACE("CP 0x{:02X}", value);

    SetZeroFlag(a == value);
    SetNegateFlag(true);
    SetHalfCarryFlag((a & 0xF) < (value & 0xF));
    SetCarryFlag(a < value);
}

void SM83::cp_dhl() {
    LTRACE("CP (HL)");

    u8 value = bus.Read8(hl);

    SetZeroFlag(a == value);
    SetNegateFlag(true);
    SetHalfCarryFlag((a & 0xF) < (value & 0xF));
    SetCarryFlag(a < value);
}

void SM83::cp_r(u8 reg) {
    SetZeroFlag(a == reg);
    SetNegateFlag(true);
    SetHalfCarryFlag((a & 0xF) < (reg & 0xF));
    SetCarryFlag(a < reg);
}

void SM83::cpl() {
    LTRACE("CPL");

    a = ~a;

    SetNegateFlag(true);
    SetHalfCarryFlag(true);
}

void SM83::daa() {
    LTRACE("DAA");
    // lifted from SameBoy.
    // this is one nutty instruction
    // TODO: come up with an original implementation

    u16 result = a;
    a = 0;

    if (HasFlag(Flags::Negate)) {
        if (HasFlag(Flags::HalfCarry)) {
            result -= 0x06;
            result &= 0xFF;
        }

        if (HasFlag(Flags::Carry)) {
            result -= 0x60;
        }
    } else {
        if (HasFlag(Flags::HalfCarry) || (result & 0x0F) >= 0x0A) {
            result += 0x06;
        }

        if (HasFlag(Flags::Carry) || result >= 0xA0) {
            result += 0x60;
        }
    }

    SetZeroFlag((result & 0xFF) == 0);
    SetHalfCarryFlag(false);
    if (Common::IsBitSet<8>(result)) {
        // Set the carry flag if the result is 16 bit
        SetCarryFlag(true);
    }

    a |= result;
}

void SM83::dec_r(u8* reg) {
    --*reg;

    SetZeroFlag(*reg == 0);
    SetNegateFlag(true);
    SetHalfCarryFlag((*reg & 0x0F) == 0x0F);
}

void SM83::dec_rr(u16* reg) {
    --*reg;

    timer.AdvanceCycles(4);
}

void SM83::dec_dhl() {
    LTRACE("DEC (HL)");

    u8 value = bus.Read8(hl);
    value--;
    bus.Write8(hl, value);

    SetZeroFlag(value == 0);
    SetNegateFlag(true);
    SetHalfCarryFlag((value & 0x0F) == 0x0F);
}

void SM83::di() {
    LTRACE("DI");
    ime = false;
    ime_delay = false;
}

void SM83::ei() {
    LTRACE("EI");
    ime_delay = true;
}

void SM83::halt() {
    LTRACE("HALT");
    halted = true;
}

void SM83::inc_r(u8* reg) {
    ++*reg;

    SetZeroFlag(*reg == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag((*reg & 0x0F) == 0x00);
}

void SM83::inc_rr(u16* reg) {
    ++*reg;

    timer.AdvanceCycles(4);
}

void SM83::inc_dhl() {
    LTRACE("INC (HL)");

    u8 value = bus.Read8(hl);
    value++;
    bus.Write8(hl, value);

    SetZeroFlag(value == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag((value & 0x0F) == 0x00);
}

template <SM83::Conditions cond>
void SM83::jp_a16() {
    const u16 addr = GetWordFromPC();

    if constexpr (cond == Conditions::None) {
        LTRACE("JP 0x{:04X}", addr);
    } else {
        LTRACE("JP {}, 0x{:04X}", GetConditionString<cond>(), addr);
    }

    if (MeetsCondition<cond>()) {
        pc = addr;
        timer.AdvanceCycles(4);
    }
}

void SM83::jp_hl() {
    LTRACE("JP HL");
    pc = hl;
}

template <SM83::Conditions cond>
void SM83::jr_r8() {
    const s8 offset = static_cast<s8>(GetByteFromPC());
    const u16 new_pc = pc + offset;

    if constexpr (cond == Conditions::None) {
        LTRACE("JR 0x{:04X}", new_pc);
    } else {
        LTRACE("JR {}, 0x{:04X}", GetConditionString<cond>(), new_pc);
    }

    if (MeetsCondition<cond>()) {
        pc = new_pc;
        timer.AdvanceCycles(4);
    }
}

void SM83::ld_da16_a() {
    u16 address = GetWordFromPC();
    LTRACE("LD (0x{:04X}), A", address);

    bus.Write8(address, a);
}

void SM83::ld_da16_sp() {
    u16 address = GetWordFromPC();
    LTRACE("LD (0x{:04X}), SP", address);

    bus.Write8(address, static_cast<u8>(sp & 0xFF));
    bus.Write8(address + 1, static_cast<u8>(sp >> 8));
}

void SM83::ld_dbc_a() {
    LTRACE("LD (BC), A");

    bus.Write8(bc, a);
}

void SM83::ld_dc_a() {
    LTRACE("LD (0xFF00+C), A");
    u16 address = 0xFF00 + c;

    bus.Write8(address, a);
}

void SM83::ld_dde_a() {
    LTRACE("LD (DE), A");
    
    bus.Write8(de, a);
}

void SM83::ld_de_d16() {
    u16 value = GetWordFromPC();
    LTRACE("LD DE, 0x{:04X}", value);

    de = value;
}

void SM83::ld_dhl_d8() {
    u8 value = GetByteFromPC();
    LTRACE("LD (HL), 0x{:02X}", value);

    bus.Write8(hl, value);
}

void SM83::ld_dhl_r(u8 reg) {
    bus.Write8(hl, reg);
}

void SM83::ld_dhld_a() {
    LTRACE("LD (HL-), A");
        
    bus.Write8(hl--, a);
}

void SM83::ld_dhli_a() {
    LTRACE("LD (HL+), A");

    bus.Write8(hl++, a);
}

void SM83::ld_a_da16() {
    u16 addr = GetWordFromPC();
    LTRACE("LD A, (0x{:04X})", addr);

    a = bus.Read8(addr);
}

void SM83::ld_a_dbc() {
    LTRACE("LD A, (BC)");
    a = bus.Read8(bc);
}

void SM83::ld_a_dc() {
    LTRACE("LD A, (0xFF00+C)");
    a = bus.Read8(0xFF00 + c);
}

void SM83::ld_a_dde() {
    LTRACE("LD A, (DE)");
    a = bus.Read8(de);
}

void SM83::ld_a_dhld() {
    LTRACE("LD A, (HL-)");
    a = bus.Read8(hl--);
}

void SM83::ld_a_dhli() {
    LTRACE("LD A, (HL+)");
    a = bus.Read8(hl++);
}

void SM83::ld_bc_d16() {
    u16 value = GetWordFromPC();
    LTRACE("LD BC, 0x{:04X}", value);

    bc = value;
}

void SM83::ld_hl_d16() {
    u16 value = GetWordFromPC();
    LTRACE("LD HL, 0x{:04X}", value);

    hl = value;
}

template <SM83::Registers DestRegister, SM83::Registers SrcRegister>
void SM83::ld_r_r() {
    LTRACE("LD {}, {}", Get8bitRegisterName<DestRegister>(), Get8bitRegisterName<SrcRegister>());

    u8* dest = Get8bitRegisterPointer<DestRegister>();
    *dest = Get8bitRegister<SrcRegister>();
}

template <SM83::Registers Register>
void SM83::ld_r_d8() {
    const u8 value = GetByteFromPC();
    LTRACE("LD {}, 0x{:02X}", Get8bitRegisterName<Register>(), value);

    u8* dest = Get8bitRegisterPointer<Register>();
    *dest = value;
}

void SM83::ld_r_dhl(u8* reg) {
    *reg = bus.Read8(hl);
}

void SM83::ld_sp_d16() {
    u16 value = GetWordFromPC();
    LTRACE("LD SP, 0x{:04X}", value);

    sp = value;
}

void SM83::ld_hl_sp_d8() {
    s8 value = static_cast<s8>(GetByteFromPC());
    LTRACE("LD HL, SP+0x{:02X}", value);

    u32 result = sp + value;

    SetZeroFlag(false);
    SetNegateFlag(false);
    SetHalfCarryFlag(((sp ^ value ^ (result & 0xFFFF)) & 0x10) == 0x10);
    SetCarryFlag(((sp ^ value ^ (result & 0xFFFF)) & 0x100) == 0x100);

    hl = static_cast<u16>(result);

    timer.AdvanceCycles(4);
}

void SM83::ld_sp_hl() {
    LTRACE("LD SP, HL");
    sp = hl;

    timer.AdvanceCycles(4);
}

void SM83::ldh_a_da8() {
    u8 offset = GetByteFromPC();
    u16 address = 0xFF00 + offset;
    LTRACE("LDH A, (0xFF00+0x{:02X})", offset);

    a = bus.Read8(address);
}

void SM83::ldh_da8_a() {
    u8 offset = GetByteFromPC();
    u16 address = 0xFF00 + offset;
    LTRACE("LDH (0xFF00+0x{:02X}), A", offset);

    bus.Write8(address, a);
}

void SM83::nop() {
    LTRACE("NOP");
}

void SM83::or_d8() {
    u8 value = GetByteFromPC();
    LTRACE("OR 0x{:02X}", value);

    a |= value;

    SetZeroFlag(a == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);
}

void SM83::or_dhl() {
    LTRACE("OR (HL)");

    a |= bus.Read8(hl);

    SetZeroFlag(a == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);
}

void SM83::or_r(u8 reg) {
    a |= reg;

    SetZeroFlag(a == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);
}

template <SM83::Registers Register>
void SM83::pop_rr() {
    LTRACE("POP {}", Get16bitRegisterName<Register>());
    StackPop(Get16bitRegisterPointer<Register>());

    if constexpr (Register == Registers::AF) {
        // Lowest 4 bits are always 0
        f &= ~0x0F;
    }
}

template <SM83::Registers Register>
void SM83::push_rr() {
    LTRACE("PUSH {}", Get16bitRegisterName<Register>());
    StackPush(Get16bitRegister<Register>());

    timer.AdvanceCycles(4);
}

template <u8 Bit, SM83::Registers Register>
void SM83::res() {
    LTRACE("RES {}, {}", Bit, Get8bitRegisterName<Register>());

    u8* reg = Get8bitRegisterPointer<Register>();
    Common::ResetBits<Bit>(*reg);
}

void SM83::res_dhl(u8 bit) {
    LTRACE("RES {}, (HL)", bit);
    u8 value = bus.Read8(hl);

    value &= ~(1 << bit);
    bus.Write8(hl, value);
}

template <SM83::Conditions cond>
void SM83::ret() {
    if constexpr (cond == Conditions::None) {
        LTRACE("RET");
        
        StackPop(&pc);
        timer.AdvanceCycles(4);
    } else {
        LTRACE("RET {}", GetConditionString<cond>());

        if (MeetsCondition<cond>()) {
            StackPop(&pc);
            timer.AdvanceCycles(8);
        } else {
            timer.AdvanceCycles(4);
        }
    }
}

void SM83::reti() {
    LTRACE("RETI");

    StackPop(&pc);
    ime = true;

    timer.AdvanceCycles(4);
}

void SM83::rl_dhl() {
    LTRACE("RL (HL)");

    u8 value = bus.Read8(hl);
    bool carry = HasFlag(Flags::Carry);

    bool should_carry = value & (1 << 7);
    SetCarryFlag(should_carry);

    u8 result = value << 1;
    result |= carry;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);

    bus.Write8(hl, result);
}

void SM83::rl_r(u8* reg) {
    bool carry = HasFlag(Flags::Carry);

    bool should_carry = *reg & (1 << 7);
    SetCarryFlag(should_carry);

    u8 result = *reg << 1;
    result |= carry;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);

    *reg = result;
}

void SM83::rla() {
    LTRACE("RLA");

    bool carry = HasFlag(Flags::Carry);

    bool should_carry = a & (1 << 7);
    SetCarryFlag(should_carry);

    u8 result = a << 1;
    result |= carry;

    SetZeroFlag(false);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);

    a = result;
}

void SM83::rlc_dhl() {
    LTRACE("RLC (HL)");

    u8 reg = bus.Read8(hl);
    u8 result = reg << 1;
    bool should_carry = reg & (1 << 7);

    SetZeroFlag(reg == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    bus.Write8(hl, result | should_carry);
}

void SM83::rlc_r(u8* reg) {
    u8 result = *reg << 1;
    bool should_carry = *reg & (1 << 7);

    SetZeroFlag(*reg == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    *reg = (result | should_carry);
}

void SM83::rlca() {
    LTRACE("RLCA");
    u8 result = a << 1;
    bool should_carry = a & (1 << 7);

    SetZeroFlag(false);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    a = (result | should_carry);
}

void SM83::rr_dhl() {
    LTRACE("RR (HL)");

    u8 value = bus.Read8(hl);
    bool carry = HasFlag(Flags::Carry);

    SetCarryFlag(value & 0x1);

    u8 result = value >> 1;
    result |= (carry << 7);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);

    bus.Write8(hl, result);
}

void SM83::rr_r(u8* reg) {
    bool carry = HasFlag(Flags::Carry);

    SetCarryFlag(*reg & 0x1);

    u8 result = *reg >> 1;
    result |= (carry << 7);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);

    *reg = result;
}

void SM83::rra() {
    LTRACE("RRA");
    bool carry = HasFlag(Flags::Carry);

    u8 result = a >> 1;
    result |= (carry << 7);

    SetZeroFlag(false);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(a & 0x1);

    a = result;
}

void SM83::rrc_dhl() {
    LTRACE("RRC (HL)");

    u8 reg = bus.Read8(hl);
    bool should_carry = reg & 0x1;
    u8 result = (reg >> 1) | (should_carry << 7);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    bus.Write8(hl, result);
}

void SM83::rrc_r(u8* reg) {
    bool should_carry = *reg & 0x1;
    u8 result = (*reg >> 1) | (should_carry << 7);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    *reg = result;
}

void SM83::rrca() {
    LTRACE("RRCA");
    bool carry = a & 0x1;
    u8 result = static_cast<u8>((a >> 1) | (carry << 7));

    SetZeroFlag(false);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(carry);

    a = result;
}

void SM83::swap_dhl() {
    LTRACE("SWAP (HL)");

    u8 value = bus.Read8(hl);
    u8 low = value & 0x0F;
    u8 high = (value & 0xF0) >> 4;

    u8 result = static_cast<u8>((low << 4) | high);

    bus.Write8(hl, result);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);
}

void SM83::swap_r(u8* reg) {
    u8 low = *reg & 0x0F;
    u8 high = (*reg & 0xF0) >> 4;

    u8 result = static_cast<u8>((low << 4) | high);

    *reg = result;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);
}

void SM83::rst(u8 addr) {
    LTRACE("RST 0x{:02X}", addr);

    if (addr == 0x0038 && bus.Read8(0x0038, false) == 0xFF) {
        LFATAL("Stack overflow");
        DumpRegisters();
        bus.DumpMemoryToFile();
        std::exit(0);
    }

    StackPush(pc);
    pc = static_cast<u16>(addr);

    timer.AdvanceCycles(4);
}

void SM83::sbc_a_d8() {
    u8 value = GetByteFromPC();
    LTRACE("SBC A, 0x{:02X}", value);

    bool carry = HasFlag(Flags::Carry);
    u16 full = a - value - carry;
    u8 result = static_cast<u8>(full);

    SetZeroFlag(result == 0);
    SetNegateFlag(true);
    SetHalfCarryFlag(((a & 0xF) < (value & 0xF) + carry) != 0);
    SetCarryFlag(full > 0xFF);

    a = result;
}

void SM83::sbc_dhl() {
    LTRACE("SBC A, (HL)");
    sbc_r(bus.Read8(hl));
}

void SM83::sbc_r(u8 reg) {
    bool carry = HasFlag(Flags::Carry);
    u16 full = a - reg - carry;
    u8 result = static_cast<u8>(full);

    SetZeroFlag(result == 0);
    SetNegateFlag(true);
    SetHalfCarryFlag(((a & 0xF) < (reg & 0xF) + carry) != 0);
    SetCarryFlag(full > 0xFF);

    a = result;
}

void SM83::scf() {
    LTRACE("SCF");

    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(true);
}

template <u8 Bit>
void SM83::set_dhl() {
    LTRACE("SET {}, (HL)", Bit);
    u8 value = bus.Read8(hl);

    Common::SetBits<Bit>(value);
    bus.Write8(hl, value);
}

template <u8 Bit, SM83::Registers Register>
void SM83::set() {
    LTRACE("SET {}, {}", Bit, Get8bitRegisterName<Register>());

    u8* reg = Get8bitRegisterPointer<Register>();
    Common::SetBits<Bit>(*reg);
}

void SM83::sla_dhl() {
    LTRACE("SLA (HL)");

    u8 value = bus.Read8(hl);
    bool should_carry = Common::IsBitSet<7>(value);
    u8 result = value << 1;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    bus.Write8(hl, result);
}

void SM83::sla_r(u8* reg) {
    bool should_carry = Common::IsBitSet<7>(*reg);
    u8 result = *reg << 1;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(should_carry);

    *reg = result;
}

void SM83::sra_dhl() {
    LTRACE("SRA (HL)");

    u8 value = bus.Read8(hl);
    u8 bit7 = value & (1 << 7);
    u8 result = value >> 1;

    SetZeroFlag((result | bit7) == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(value & 0x1);

    bus.Write8(hl, result | bit7);
}

void SM83::sra_r(u8* reg) {
    u8 bit7 = *reg & (1 << 7);
    u8 result = *reg >> 1;

    SetZeroFlag((result | bit7) == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(*reg & 0x1);

    *reg = (result | bit7);
}

void SM83::srl_dhl() {
    LTRACE("SRL (HL)");

    u8 value = bus.Read8(hl);
    u8 result = value >> 1;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(value & 0x1);

    bus.Write8(hl, result);
}

void SM83::srl_r(u8* reg) {
    u8 result = *reg >> 1;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(*reg & 0x1);

    *reg = result;
}

void SM83::sub_r(u8 reg) {
    SetZeroFlag(a == reg);
    SetNegateFlag(true);
    SetHalfCarryFlag((a & 0xF) < (reg & 0xF));
    SetCarryFlag(a < reg);

    a -= reg;
}

void SM83::sub_d8() {
    u8 value = GetByteFromPC(); 
    LTRACE("SUB 0x{:02X}", value);

    SetZeroFlag(a == value);
    SetNegateFlag(true);
    SetHalfCarryFlag((a & 0xF) < (value & 0xF));
    SetCarryFlag(a < value);

    a -= value;
}

void SM83::xor_d8() {
    u8 value = GetByteFromPC();
    LTRACE("XOR 0x{:02X}", value);

    u8 result = a ^ value;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);

    a = result;
}

void SM83::xor_dhl() {
    LTRACE("XOR (HL)");

    u8 result = a ^ bus.Read8(hl);

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);

    a = result;
}

void SM83::xor_r(u8 reg) {
    u8 result = a ^ reg;

    SetZeroFlag(result == 0);
    SetNegateFlag(false);
    SetHalfCarryFlag(false);
    SetCarryFlag(false);

    a = result;
}
