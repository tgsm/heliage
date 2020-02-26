#pragma once

#include "bus.h"
#include "types.h"

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

    SM83(Bus& bus);

    void Reset();
    u8 Tick();

    u8 GetByteFromPC();
    u16 GetWordFromPC();

    void SetZeroFlag(bool b);
    void SetNegateFlag(bool b);
    void SetHalfCarryFlag(bool b);
    void SetCarryFlag(bool b);

    bool HasFlag(Flags flag);

    void StackPush(u16* word_reg);
    void StackPop(u16* word_reg);

    bool ExecuteOpcode(const u8 opcode, u16 pc_at_opcode);
    bool ExecuteCBOpcode(const u8 opcode, u16 pc_at_opcode);
    void HandleInterrupts();

    void AdvanceCycles(u8 cycles);

    void DumpRegisters();
private:
    union {
        u16 af;
        struct { u8 f; u8 a; }; // little endian
    };

    union {
        u16 bc;
        struct { u8 c; u8 b; };
    };

    union {
        u16 de;
        struct { u8 e; u8 d; };
    };

    union {
        u16 hl;
        struct { u8 l; u8 h; };
    };

    // stack pointer
    u16 sp;

    // program counter
    u16 pc;
    u16 pc_at_opcode; // used for debugging

    u8 cycles_to_advance;

    bool ime;
    bool ime_delay; // EI enables interrupts one instruction after

    Bus& bus;

    // illegal instruction
    void ill(const u8 opcode);

    void adc_a_d8();
    void adc_a_dhl();
    void adc_a_r(u8 reg);

    void add_a_d8();
    void add_a_dhl();
    void add_a_r(u8 reg);
    void add_hl_bc();
    void add_hl_de();
    void add_hl_hl();

    void and_d8();
    void and_r(u8 reg);

    void bit(u8 bit, u8* reg);
    void bit_dhl(u8 bit);

    void call_a16();
    void call_nc_a16();
    void call_nz_a16();

    void ccf();

    void cp_d8();
    void cp_dhl();
    void cp_r(u8 reg);

    void cpl();

    void daa();

    void dec_a();
    void dec_b();
    void dec_bc();
    void dec_c();
    void dec_d();
    void dec_de();
    void dec_dhl();
    void dec_e();
    void dec_h();
    void dec_hl();
    void dec_l();

    void di();

    void ei();

    void inc_a();
    void inc_b();
    void inc_bc();
    void inc_c();
    void inc_d();
    void inc_de();
    void inc_dhl();
    void inc_e();
    void inc_h();
    void inc_hl();
    void inc_l();
    void inc_sp();

    void jp_a16();
    void jp_hl();
    void jp_nz_a16();
    void jp_z_a16();

    void jr_r8();
    void jr_c_r8();
    void jr_nc_r8();
    void jr_nz_r8();
    void jr_z_r8();

    void ld_a_a();
    void ld_a_b();
    void ld_a_c();
    void ld_a_d();
    void ld_a_d8();
    void ld_a_da16();
    void ld_a_dbc();
    void ld_a_dde();
    void ld_a_dhl();
    void ld_a_dhld();
    void ld_a_dhli();
    void ld_a_e();
    void ld_a_h();
    void ld_a_l();
    void ld_b_a();
    void ld_b_b();
    void ld_b_c();
    void ld_b_d();
    void ld_b_d8();
    void ld_b_dhl();
    void ld_b_e();
    void ld_b_h();
    void ld_b_l();
    void ld_bc_d16();
    void ld_c_a();
    void ld_c_b();
    void ld_c_c();
    void ld_c_d();
    void ld_c_d8();
    void ld_c_dhl();
    void ld_c_e();
    void ld_c_h();
    void ld_c_l();
    void ld_d_a();
    void ld_d_b();
    void ld_d_c();
    void ld_d_d();
    void ld_d_d8();
    void ld_d_dhl();
    void ld_d_e();
    void ld_d_h();
    void ld_d_l();
    void ld_da16_a();
    void ld_da16_sp();
    void ld_dbc_a();
    void ld_dc_a();
    void ld_dde_a();
    void ld_de_d16();
    void ld_dhl_a();
    void ld_dhl_b();
    void ld_dhl_c();
    void ld_dhl_d();
    void ld_dhl_d8();
    void ld_dhl_e();
    void ld_dhl_h();
    void ld_dhl_l();
    void ld_dhld_a();
    void ld_dhli_a();
    void ld_e_a();
    void ld_e_b();
    void ld_e_c();
    void ld_e_d();
    void ld_e_d8();
    void ld_e_dhl();
    void ld_e_e();
    void ld_e_h();
    void ld_e_l();
    void ld_h_a();
    void ld_h_b();
    void ld_h_c();
    void ld_h_d();
    void ld_h_d8();
    void ld_h_dhl();
    void ld_h_e();
    void ld_h_h();
    void ld_h_l();
    void ld_hl_d16();
    void ld_hl_sp_d8();
    void ld_l_a();
    void ld_l_b();
    void ld_l_c();
    void ld_l_d();
    void ld_l_d8();
    void ld_l_dhl();
    void ld_l_e();
    void ld_l_h();
    void ld_l_l();
    void ld_sp_d16();
    void ld_sp_hl();
    void ldh_a_da8();
    void ldh_da8_a();

    void nop();

    void or_d8();
    void or_dhl();
    void or_r(u8 reg);

    void pop_af();
    void pop_bc();
    void pop_de();
    void pop_hl();

    void push_af();
    void push_bc();
    void push_de();
    void push_hl();

    void res(u8 bit, u8* reg);
    void res_dhl(u8 bit);

    void ret();
    void ret_c();
    void ret_nc();
    void ret_z();
    void ret_nz();

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

    void set_dhl(u8 bit);
    void set(u8 bit, u8* reg);

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