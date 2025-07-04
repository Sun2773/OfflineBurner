#include "flash_blob.h"

/*
 * STM32F40xxx_41xxx_OPT SRAM布局 (基址0x20000000):
 *
 * 0x20000000 ┌─────────────────┐
 *            │ Flash Algorithm │  <- algo_start (算法代码)
 *            │    Code         │
 * 0x20000400 ├─────────────────┤
 *            │ Program Buffer  │  <- program_buffer (数据缓冲区)
 *            │  (1024 bytes)   │
 * 0x20000800 ├─────────────────┤
 *            │  Static Data    │  <- static_base (全局/静态变量)
 *            │     Area        │
 * 0x20000C00 ├─────────────────┤
 *            │     Stack       │  <- stack_pointer (栈空间)
 *            │   (grows down)  │
 *            │                 │
 *            │ ............... │
 *
 */

// Flash programming algorithm code
// Functions:
//   GetSecNum    @ +0x0005 (size: 38 bytes)
//   Init         @ +0x002B (size: 42 bytes)
//   UnInit       @ +0x0055 (size: 14 bytes)
//   EraseChip    @ +0x0063 (size: 44 bytes)
//   EraseSector  @ +0x008F (size: 4 bytes)
//   ProgramPage  @ +0x0093 (size: 48 bytes)
//   Verify       @ +0x00C3 (size: 22 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x03004601, 0x28200E00, 0x0940D302, 0xE0051D00,  // +0x0004
    0xD3022810, 0x1CC00900, 0x0880E000, 0xD50102C9,  // +0x0014
    0x43082110, 0x482C4770, 0x6081492A, 0x6081492B,  // +0x0024
    0x22F068C1, 0x60C14311, 0x06806940, 0x4829D406,  // +0x0034
    0x60014927, 0x60412106, 0x60814927, 0x47702000,  // +0x0044
    0x69414821, 0x43112201, 0x20006141, 0x481E4770,  // +0x0054
    0x21F068C2, 0x60C2430A, 0x61424A20, 0x23026942,  // +0x0064
    0x6142431A, 0x061268C2, 0xD0040F12, 0x430A68C2,  // +0x0074
    0x200160C2, 0x20004770, 0x20004770, 0x48124770,  // +0x0084
    0x68C36811, 0x431322F0, 0x4B1560C3, 0x1C894019,  // +0x0094
    0x68C16141, 0xD4FC03C9, 0x060968C1, 0xD0040F09,  // +0x00A4
    0x431168C1, 0x200160C1, 0x20004770, 0xB5104770,  // +0x00B4
    0x68124C05, 0x69644B0A, 0x401C401A, 0xD10042A2,  // +0x00C4
    0xBD101840, 0x08192A3B, 0x40023C00, 0x4C5D6E7F,  // +0x00D4
    0x00005555, 0x40003000, 0x00000FFF, 0x0FFFAAEC,  // +0x00E4
    0x0FFFFFFC, 0x00000000,                          // +0x00F4
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0004, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f40xxx_41xxx_opt_ = {
    0x2000002B,   // Init
    0x20000055,   // UnInit
    0x20000063,   // EraseChip
    0x2000008F,   // EraseSector
    0x20000093,   // ProgramPage
    {
        0x20000001,   // BKPT : 断点地址 (算法起始+1，Thumb模式)
        0x20000800,   // RSB  : 静态数据基址
        0x20000C00,   // RSP  : 栈指针地址
    },
    0x20000400,           // 编程缓冲区地址
    0x20000000,           // 算法代码起始地址
    sizeof(flash_code),   // 算法代码大小
    flash_code,           // 算法代码数据指针
    0x00000400,           // 编程缓冲区大小

    sector_info,                                    // 扇区信息指针
    sizeof(sector_info) / sizeof(sector_info[0]),   // 扇区数量
};