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
//   Init         @ +0x0005 (size: 50 bytes)
//   UnInit       @ +0x0037 (size: 18 bytes)
//   EraseChip    @ +0x0049 (size: 58 bytes)
//   EraseSector  @ +0x0083 (size: 62 bytes)
//   ProgramPage  @ +0x00C1 (size: 90 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4603B510, 0x4C442000, 0x48446020, 0x48446060,  // +0x0004
    0x46206060, 0xF01069C0, 0xD1080F04, 0x5055F245,  // +0x0014
    0x60204C40, 0x60602006, 0x70FFF640, 0x200060A0,  // +0x0024
    0x4601BD10, 0x69004838, 0x0080F040, 0x61104A36,  // +0x0034
    0x47702000, 0x69004834, 0x0004F040, 0x61084932,  // +0x0044
    0x69004608, 0x0040F040, 0xE0036108, 0x20AAF64A,  // +0x0054
    0x60084930, 0x68C0482C, 0x0F01F010, 0x482AD1F6,  // +0x0064
    0xF0206900, 0x49280004, 0x20006108, 0x46014770,  // +0x0074
    0x69004825, 0x0002F040, 0x61104A23, 0x61414610,  // +0x0084
    0xF0406900, 0x61100040, 0xF64AE003, 0x4A2120AA,  // +0x0094
    0x481D6010, 0xF01068C0, 0xD1F60F01, 0x6900481A,  // +0x00A4
    0x0002F020, 0x61104A18, 0x47702000, 0x4603B510,  // +0x00B4
    0xF0201C48, 0xE0220101, 0x69004813, 0x0001F040,  // +0x00C4
    0x61204C11, 0x80188810, 0x480FBF00, 0xF01068C0,  // +0x00D4
    0xD1FA0F01, 0x6900480C, 0x0001F020, 0x61204C0A,  // +0x00E4
    0x68C04620, 0x0F14F010, 0x4620D006, 0xF04068C0,  // +0x00F4
    0x60E00014, 0xBD102001, 0x1C921C9B, 0x29001E89,  // +0x0104
    0x2000D1DA, 0x0000E7F7, 0x40022000, 0x45670123,  // +0x0114
    0xCDEF89AB, 0x40003000, 0x00000000,              // +0x0124
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_512_ = {
    0x20000005,   // Init
    0x20000037,   // UnInit
    0x20000049,   // EraseChip
    0x20000083,   // EraseSector
    0x200000C1,   // ProgramPage
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