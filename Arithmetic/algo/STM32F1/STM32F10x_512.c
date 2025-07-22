#include "flash_blob.h"

/*
 * STM32F10x_512 SRAM布局 (基址0x20000000):
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
//   Init         @ +0x0005 (size: 60 bytes)
//   UnInit       @ +0x0041 (size: 18 bytes)
//   EraseChip    @ +0x0053 (size: 60 bytes)
//   EraseSector  @ +0x008F (size: 64 bytes)
//   ProgramPage  @ +0x00CF (size: 92 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4603B510, 0x04C00CD8, 0x444C4C47, 0x20006020,  // +0x0004
    0x60204C46, 0x60604846, 0x60604846, 0x69C04620,  // +0x0014
    0x0004F000, 0xF245B940, 0x4C435055, 0x20066020,  // +0x0024
    0xF6406060, 0x60A070FF, 0xBD102000, 0x483B4601,  // +0x0034
    0xF0406900, 0x4A390080, 0x20006110, 0x48374770,  // +0x0044
    0xF0406900, 0x49350004, 0x46086108, 0xF0406900,  // +0x0054
    0x61080040, 0xF64AE003, 0x493320AA, 0x482F6008,  // +0x0064
    0xF00068C0, 0x28000001, 0x482CD1F5, 0xF0206900,  // +0x0074
    0x492A0004, 0x20006108, 0x46014770, 0x69004827,  // +0x0084
    0x0002F040, 0x61104A25, 0x61414610, 0xF0406900,  // +0x0094
    0x61100040, 0xF64AE003, 0x4A2320AA, 0x481F6010,  // +0x00A4
    0xF00068C0, 0x28000001, 0x481CD1F5, 0xF0206900,  // +0x00B4
    0x4A1A0002, 0x20006110, 0xB5104770, 0x1C484603,  // +0x00C4
    0x0101F020, 0x4815E023, 0xF0406900, 0x4C130001,  // +0x00D4
    0x88106120, 0xBF008018, 0x68C04810, 0x0001F000,  // +0x00E4
    0xD1F92800, 0x6900480D, 0x0001F020, 0x61204C0B,  // +0x00F4
    0x68C04620, 0x0014F000, 0x4620B130, 0xF04068C0,  // +0x0104
    0x60E00014, 0xBD102001, 0x1C921C9B, 0x29001E89,  // +0x0114
    0x2000D1D9, 0x0000E7F7, 0x00000004, 0x40022000,  // +0x0124
    0x45670123, 0xCDEF89AB, 0x40003000, 0x00000000,  // +0x0134
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_512_ = {
    0x20000005,   // Init
    0x20000041,   // UnInit
    0x20000053,   // EraseChip
    0x2000008F,   // EraseSector
    0x200000CF,   // ProgramPage
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