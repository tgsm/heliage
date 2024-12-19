#pragma once

#include <bit>
#include <string_view>
#include "bus.h"
#include "common/types.h"

class SM83 {
public:
    enum class Flags : u8 {
        None = 0,
        Zero = 1 << 7,
        Negate = 1 << 6,
        HalfCarry = 1 << 5,
        Carry = 1 << 4,
    };

    enum class InterruptAddresses : u16 {
        VBlank = 0x0040,
        LCDCStatus = 0x0048,
        Timer = 0x0050,
        Serial = 0x0058,
        Joypad = 0x0060,
    };

    SM83(Bus& bus, Timer& timer);

    void Tick();

    void DumpRegisters();
private:
    static_assert(std::endian::native == std::endian::little, "Only little-endian hosts are supported at the moment");

    union {
        u16 af = 0x0000;
        struct { u8 f; u8 a; };
    };

    union {
        u16 bc = 0x0000;
        struct { u8 c; u8 b; };
    };

    union {
        u16 de = 0x0000;
        struct { u8 e; u8 d; };
    };

    union {
        u16 hl = 0x0000;
        struct { u8 l; u8 h; };
    };

    // stack pointer
    u16 sp = 0x0000;

    // program counter
    u16 pc = 0x0000;
    u16 pc_at_opcode = 0x0000; // used for debugging

    bool ime = false;
    bool ime_delay = false; // EI enables interrupts one instruction after

    bool halted = false;

    Bus& bus;
    Timer& timer;

    u8 GetByteFromPC();
    u16 GetWordFromPC();

    void SetZeroFlag(bool b);
    void SetNegateFlag(bool b);
    void SetHalfCarryFlag(bool b);
    void SetCarryFlag(bool b);

    bool HasFlag(Flags flag) const;

    enum class Conditions {
        None,
        C,
        NC,
        Z,
        NZ,
    };

    template <Conditions cond>
    bool MeetsCondition() const;

    template <Conditions cond>
    constexpr std::string_view GetConditionString() const;

    enum class Registers {
        A,
        F,
        B,
        C,
        D,
        E,
        H,
        L,

        AF,
        BC,
        DE,
        HL,
        SP,
    };

    template <Registers Register>
    constexpr char Get8bitRegisterName() const;

    template <Registers Register>
    u8* Get8bitRegisterPointer();

    template <Registers Register>
    u8 Get8bitRegister() const;

    template <Registers Register>
    constexpr std::string_view Get16bitRegisterName() const;

    template <Registers Register>
    u16* Get16bitRegisterPointer();

    template <Registers Register>
    u16 Get16bitRegister() const;

    void StackPush(u16 word_reg);
    void StackPop(u16* word_reg);

    bool ExecuteOpcode(const u8 opcode);
    bool ExecuteCBOpcode(const u8 opcode);
    void HandleInterrupts();

    // illegal instruction
    void ill(const u8 opcode);

    void adc_a_d8();
    void adc_a_r(u8 reg);

    void add_a_d8();
    void add_a_dhl();
    void add_a_r(u8 reg);
    void add_hl_bc();
    void add_hl_de();
    void add_hl_hl();
    void add_hl_sp();
    void add_sp_d8();

    void and_d8();
    void and_r(u8 reg);

    template <u8 Bit, Registers Register>
    void bit();
    template <u8 Bit>
    void bit_dhl();

    template <Conditions cond>
    void call_a16();

    void ccf();

    void cp_d8();
    void cp_dhl();
    void cp_r(u8 reg);

    void cpl();

    void daa();

    void dec_dhl();
    void dec_r(u8* reg);
    void dec_rr(u16* reg);

    void di();

    void ei();

    void halt();

    void inc_dhl();
    void inc_r(u8* reg);
    void inc_rr(u16* reg);

    template <Conditions cond>
    void jp_a16();

    void jp_hl();

    template <Conditions cond>
    void jr_r8();

    void ld_a_da16();
    void ld_a_dbc();
    void ld_a_dc();
    void ld_a_dde();
    void ld_a_dhld();
    void ld_a_dhli();
    void ld_bc_d16();
    void ld_da16_a();
    void ld_da16_sp();
    void ld_dbc_a();
    void ld_dc_a();
    void ld_dde_a();
    void ld_de_d16();
    void ld_dhl_d8();
    void ld_dhl_r(u8 reg);
    void ld_dhld_a();
    void ld_dhli_a();
    void ld_hl_d16();
    void ld_hl_sp_d8();

    template <Registers DestRegister, Registers SrcRegister>
    void ld_r_r();
    template <Registers Register>
    void ld_r_d8();
    void ld_r_dhl(u8* reg);

    void ld_sp_d16();
    void ld_sp_hl();
    void ldh_a_da8();
    void ldh_da8_a();

    void nop();

    void or_d8();
    void or_dhl();
    void or_r(u8 reg);

    template <Registers Register>
    void pop_rr();

    template <Registers Register>
    void push_rr();

    template <u8 Bit, Registers Register>
    void res();
    void res_dhl(u8 bit);

    template <Conditions cond>
    void ret();

    void reti();

    void rl_dhl();
    void rl_r(u8* reg);

    void rla();

    void rlc_dhl();
    void rlc_r(u8* reg);

    void rlca();

    void rr_dhl();
    void rr_r(u8* reg);

    void rra();

    void rrc_dhl();
    void rrc_r(u8* reg);

    void rrca();

    void rst(u8 addr);

    void sbc_a_d8();
    void sbc_dhl();
    void sbc_r(u8 reg);

    void scf();

    template <u8 Bit>
    void set_dhl();
    template <u8 Bit, Registers Register>
    void set();

    void sla_dhl();
    void sla_r(u8* reg);

    void sra_dhl();
    void sra_r(u8* reg);

    void srl_dhl();
    void srl_r(u8* reg);

    void sub_d8();
    void sub_r(u8 reg);

    void swap_dhl();
    void swap_r(u8* reg);

    void xor_d8();
    void xor_dhl();
    void xor_r(u8 reg);
};
