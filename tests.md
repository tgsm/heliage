This file documents which tests pass and fail on heliage.

Some of these tests will always fail due to currently unimplemented features, like sound or CGB support. 

# blargg
## cgb_sound
| Test | Result |
| ---- | ------ |
| 01-registers | Failed (NR10-NR51 wave RAM write/read) |
| 02-len ctr | Failed (Length becoming 0 should clear status) |
| 03-trigger | Failed (Enabling in second half of length period shouldn't clock length) |
| 04-sweep | Failed (If shift=0, doesn't calculate on trigger) |
| 05-sweep details | Failed (Timer treats period 0 as 8) |
| 06-overflow on trigger | Failed<br>(7FFF 7FFF 7FFF 7FFF 7FFF 7FFF 7FFF<br>8D7112A4) |
| 07-len sweep period sync | Failed (Length period is wrong) |
| 08-len ctr during power | Failed (00 00 00 00 2144DF1C) |
| 09-wave read while on | Failed (00 (62x) 7A75E7B6) |
| 10-wave trigger while on | Passed |
| 11-regs after power | Failed (Powering off should clear NR13) |
| 12-wave | Failed (Timer period or phase resetting is wrong) |

## cpu_instrs
| Test | Result |
| ---- | ------ |
| 01-special | Passed |
| 02-interrupts | Failed (Timer doesn't work)
| 03-op sp,hl | Passed |
| 04-op r,imm | Passed |
| 05-op rp | Passed |
| 06-ld r,r | Passed |
| 07-jr,jp,call,ret,rst | Passed |
| 08-misc instrs | Passed |
| 09-op r,r | Passed |
| 10-bit ops | Passed |
| 11-op a,(hl) | Passed |

## dmg_sound
| Test | Result |
| ---- | ------ |
| 01-registers | Failed (NR10-NR51 and wave RAM write/read) |
| 02-len ctr | Failed (Length becoming 0 should clear status) |
| 03-trigger | Failed (Enabling in second half of length period shouldn't clock length) |
| 04-sweep | Failed (If shift=0, doesn't calculate on trigger) |
| 05-sweep details | Failed (Timer treats period 0 as 8) |
| 06-overflow on trigger | Failed<br>(7FFF 7FFF 7FFF 7FFF 7FFF 7FFF 7FFF<br>8D7112A4) |
| 07-len sweep period sync | Failed (Length period is wrong) |
| 08-len ctr during power | Failed (00 00 00 00 2144DF1C) |
| 09-wave read while on | Failed (00 (62x) 7A75E7B6) |
| 10-wave trigger while on | Failed |
| 11-regs after power | Failed (Powering off should clear NR13) |
| 12-wave write while on | Failed (Timer period or phase resetting is wrong) |

## halt_bug
Failed (3DB103C3)

## instr_timing
Failed (#255)

## interrupt_time
Failed

## mem_timing
| Test | Result |
| ---- | ------ |
| 01-read_timing | Failed (freezes before completion) |
| 02-write_timing | Failed (freezes before completion) |
| 03-modify_timing | Failed (freezes before completion) |

## mem_timing-2
| Test | Result |
| ---- | ------ |
| 01-read_timing | Failed (freezes before completion) |
| 02-write_timing | Failed (freezes before completion) |
| 03-modify_timing | Failed (freezes before completion) |

## oam_bug
| Test | Result |
| ---- | ------ |
| 1-lcd_sync | Failed (Turning LCD on starts too late in scanline) |
| 2-causes | Failed (LD DE,$FE00 : INC DE) |
| 3-non_causes | Passed |
| 4-scanline_timing | Failed (INC DE at first corruption) |
| 5-timing_bug | Failed (Should corrupt at beginning of first scanline)
| 6-timing_no_bug | Passed |
| 7-timing_effect | Failed (00000000) |
| 8-instr_effect | Failed<br>(00000000<br>INC/DEC rp pattern is wrong) |

# Mooneye
## acceptance
| Test | Result |
| ---- | ------ |
| add_sp_e_timing | Failed |
| bits/mem_oam | Passed |
| bits/reg_f | Passed |
| bits/unused_hwio-GS | Failed |
| boot_div2-S | Failed |
| boot_div-dmg0 | Failed |
| boot_div-dmgABCmgb | Failed |
| boot_div-S | Failed |
| boot_hwio-dmg0 | Failed |
| boot_hwio-dmgABCmgb | Failed |
| boot_hwio-S | Failed |
| boot_regs-dmg0 | Passed |
| boot_regs-dmgABC | Passed |
| boot_regs-mgb | Passed |
| boot_regs-sgb2 | Passed |
| boot_regs-sgb | Passed |
| call_cc_timing2 | Failed |
| call_cc_timing | Failed (Round 1) |
| call_timing2 | Failed |
| call_timing | Failed (Round 1) |
| di_timing-GS | Failed (Round 1) |
| div_timing | Failed |
| ei_sequence | Passed |
| ei_timing | Passed |
| halt_ime0_ei | Passed |
| halt_ime0_nointr_timing | Failed |
| halt_ime1_timing2-GS | Failed (Round 1) |
| halt_ime1_timing | Failed (stack overflow) |
| if_ie_registers | Failed |
| instr/daa | Passed |
| interrupts/ie_push | Failed (Round 1: not cancelled) |
| intr_timing | Failed |
| jp_cc_timing | Failed (Round 1) |
| jp_timing | Failed (Round 1) |
| ld_hl_sp_e_timing | Failed |
| oam_dma/basic | Passed |
| oam_dma/reg_read | Passed |
| oam_dma_restart | Failed |
| oam_dma/sources-GS | Failed ($A000) |
| oam_dma_start | Failed |
| oam_dma_timing | Failed |
| pop_timing | Failed |
| ppu/hblank_ly_scx_timing-GS | Failed (freezes) |
| ppu/intr_1_2_timing-GS | Failed (freezes) |
| ppu/intr_2_0_timing | Failed (freezes) |
| ppu/intr_2_mode0_timing | Failed (freezes) |
| ppu/intr_2_mode0_timing_sprites | Failed (freezes) |
| ppu/intr_2_mode3_timing | Failed (freezes) |
| ppu/intr_2_oam_ok_timing | Failed (freezes) |
| ppu/lcdon_timing-GS | Failed |
| ppu/lcdon_write_timing-GS | Failed |
| ppu/stat_irq_blocking | Failed (stack overflow) |
| ppu/stat_lyc_onoff | Failed (Round 1 step 1) |
| ppu/vblank_stat_intr-GS | Failed (freezes) |
| push_timing | Failed |
| rapid_di_ei | Passed |
| ret_cc_timing | Failed (Round 1) |
| reti_intr_timing | Passed |
| reti_timing | Failed (stack overflow) |
| ret_timing | Failed (Round 1) |
| rst_timing | Failed |
| serial/boot_sclk_align-dmgABCmgb | Failed (No serial intr) |
| timer/div_write | Passed |
| timer/rapid_toggle | Failed (No intr) |
| timer/tim00_div_trigger | Failed |
| timer/tim00 | Failed |
| timer/tim01_div_trigger | Failed |
| timer/tim01 | Failed |
| timer/tim10_div_trigger | Failed |
| timer/tim10 | Failed |
| timer/tim11_div_trigger | Failed |
| timer/tim11 | Failed |
| timer/tima_reload | Failed |
| timer/tima_write_reloading | Failed |
| timer/tma_write_reloading | Failed |

## emulator-only
| Test | Result |
| ---- | ------ |
| mbc1/bits_bank1 | Passed |
| mbc1/bits_bank2 | Failed (Round 1: initial BANK2) |
| mbc1/bits_mode | Passed |
| mbc1/bits_ramg | Failed (freezes) |
| mbc1/multicart_rom_8Mb | Failed |
| mbc1/ram_256kb | Failed (stack overflow) |
| mbc1/ram_64kb | Failed (crashes heliage) |
| mbc1/rom_16Mb | Failed |
| mbc1/rom_1Mb | Failed |
| mbc1/rom_2Mb | Failed |
| mbc1/rom_4Mb | Failed |
| mbc1/rom_512kb | Failed |
| mbc1/rom_8Mb | Failed |
| mbc2/bits_ramg | Failed (Round 1: 3EFF; RAM not enabled) |
| mbc2/bits_romb | Failed (Round 2: 3FFF) |
| mbc2/bits_unused | Passed |
| mbc2/ram | Failed (Round 3) |
| mbc2/rom_1Mb | Failed |
| mbc2/rom_2Mb | Failed |
| mbc2/rom_512kb | Failed |
| mbc5/rom_16Mb | Failed |
| mbc5/rom_1Mb | Failed |
| mbc5/rom_2Mb | Failed |
| mbc5/rom_32Mb | Failed |
| mbc5/rom_4Mb | Failed |
| mbc5/rom_512kb | Failed |
| mbc5/rom_64Mb | Failed |
| mbc5/rom_8Mb | Failed |
