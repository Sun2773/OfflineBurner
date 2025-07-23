#include "flash_blob.h"

/*
 * STM32F10x_1024 SRAM布局 (基址0x20000000):
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
//   Init         @ +0x0005 (size: 68 bytes)
//   UnInit       @ +0x0049 (size: 28 bytes)
//   EraseChip    @ +0x0065 (size: 114 bytes)
//   EraseSector  @ +0x00D7 (size: 138 bytes)
//   ProgramPage  @ +0x0161 (size: 186 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4603B510, 0x04C00CD8, 0x444C4C83, 0x20006020,  // +0x0004
    0x60204C82, 0x60604882, 0x60604882, 0x64604880,  // +0x0014
    0x64604880, 0x69C04620, 0x0004F000, 0xF245B940,  // +0x0024
    0x4C7D5055, 0x20066020, 0xF6406060, 0x60A070FF,  // +0x0034
    0xBD102000, 0x48754601, 0xF0406900, 0x4A730080,  // +0x0044
    0x46106110, 0xF0406D00, 0x65100080, 0x47702000,  // +0x0054
    0x6900486E, 0x0004F040, 0x6108496C, 0x69004608,  // +0x0064
    0x0040F040, 0xE0036108, 0x20AAF64A, 0x6008496A,  // +0x0074
    0x68C04866, 0x0001F000, 0xD1F52800, 0x69004863,  // +0x0084
    0x0004F020, 0x61084961, 0x6D004608, 0x0004F040,  // +0x0094
    0x46086508, 0xF0406D00, 0x65080040, 0xF64AE003,  // +0x00A4
    0x495D20AA, 0x48596008, 0xF0006CC0, 0x28000001,  // +0x00B4
    0x4856D1F5, 0xF0206D00, 0x49540004, 0x20006508,  // +0x00C4
    0x46014770, 0x44484850, 0xF5006800, 0x42812000,  // +0x00D4
    0x484ED21D, 0xF0406900, 0x4A4C0002, 0x46106110,  // +0x00E4
    0x69006141, 0x0040F040, 0xE0036110, 0x20AAF64A,  // +0x00F4
    0x60104A49, 0x68C04845, 0x0001F000, 0xD1F52800,  // +0x0104
    0x69004842, 0x0002F020, 0x61104A40, 0x483FE01C,  // +0x0114
    0xF0406D00, 0x4A3D0002, 0x46106510, 0x6D006541,  // +0x0124
    0x0040F040, 0xE0036510, 0x20AAF64A, 0x60104A3A,  // +0x0134
    0x6CC04836, 0x0001F000, 0xD1F52800, 0x6D004833,  // +0x0144
    0x0002F020, 0x65104A31, 0x47702000, 0x4603B510,  // +0x0154
    0xF0201C48, 0x482C0101, 0x68004448, 0x2000F500,  // +0x0164
    0xD2274283, 0x4829E023, 0xF0406900, 0x4C270001,  // +0x0174
    0x88106120, 0xBF008018, 0x68C04824, 0x0001F000,  // +0x0184
    0xD1F92800, 0x69004821, 0x0001F020, 0x61204C1F,  // +0x0194
    0x68C04620, 0x0014F000, 0x4620B130, 0xF04068C0,  // +0x01A4
    0x60E00014, 0xBD102001, 0x1C921C9B, 0x29001E89,  // +0x01B4
    0xE026D1D9, 0x4815E023, 0xF0406D00, 0x4C130001,  // +0x01C4
    0x88106520, 0xBF008018, 0x6CC04810, 0x0001F000,  // +0x01D4
    0xD1F92800, 0x6D00480D, 0x0001F020, 0x65204C0B,  // +0x01E4
    0x6CC04620, 0x0014F000, 0x4620B130, 0xF0406CC0,  // +0x01F4
    0x64E00014, 0xE7D62001, 0x1C921C9B, 0x29001E89,  // +0x0204
    0x2000D1D9, 0x0000E7CF, 0x00000004, 0x40022000,  // +0x0214
    0x45670123, 0xCDEF89AB, 0x40003000, 0x00000000,  // +0x0224
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_1024_ = {
    0x20000005,   // Init
    0x20000049,   // UnInit
    0x20000065,   // EraseChip
    0x200000D7,   // EraseSector
    0x20000161,   // ProgramPage
    0x00000000,   // SetRDP
    0x00000000,   // Verify
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