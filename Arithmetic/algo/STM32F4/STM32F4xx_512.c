#include "flash_blob.h"

/*
 * STM32F4xx_512 SRAM布局 (基址0x20000000):
 *
 * 0x20000000 ┌─────────────────┐
 *            │ Flash Algorithm │  <- algo_start (算法代码)
 *            │    Code         │
 * 0x20000400 ├─────────────────┤
 *            │ Program Buffer  │  <- program_buffer (数据缓冲区)
 *            │  (1024 bytes)   │
 * 0x20000800 ├─────────────────┤
 *            │                 │
 *            │   Free Space    │
 *            │                 │
 * 0x20000C00 ├─────────────────┤
 *            │  Static Data    │  <- static_base (全局/静态变量)
 *            │     Area        │
 * 0x20001000 ├─────────────────┤
 *            │     Stack       │  <- stack_pointer (栈空间)
 *            │   (grows down)  │
 *            └─────────────────┘
 */


// Flash programming algorithm code
// Functions:
//   GetSecNum    @ +0x0005 (size: 38 bytes)
//   Init         @ +0x002B (size: 46 bytes)
//   UnInit       @ +0x0059 (size: 14 bytes)
//   EraseChip    @ +0x0067 (size: 44 bytes)
//   EraseSector  @ +0x0093 (size: 76 bytes)
//   ProgramPage  @ +0x00DF (size: 82 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x03004601, 0x28200E00, 0x0940D302, 0xE0051D00,  // +0x0004
    0xD3022810, 0x1CC00900, 0x0880E000, 0xD50102C9,  // +0x0014
    0x43082110, 0x48424770, 0x60414940, 0x60414941,  // +0x0024
    0x60012100, 0x22F068C1, 0x60C14311, 0x06806940,  // +0x0034
    0x483ED406, 0x6001493C, 0x60412106, 0x6081493C,  // +0x0044
    0x47702000, 0x69014836, 0x43110542, 0x20006101,  // +0x0054
    0xB5104770, 0x69014832, 0x43212404, 0x69016101,  // +0x0064
    0x431103A2, 0x49336101, 0xE0004A30, 0x68C36011,  // +0x0074
    0xD4FB03DB, 0x43A16901, 0x20006101, 0xB530BD10,  // +0x0084
    0xFFB6F7FF, 0x68CA4926, 0x431A23F0, 0x240260CA,  // +0x0094
    0x690A610C, 0x0E0006C0, 0x610A4302, 0x03E26908,  // +0x00A4
    0x61084310, 0x4A214823, 0x6010E000, 0x03ED68CD,  // +0x00B4
    0x6908D4FB, 0x610843A0, 0x060068C8, 0xD0030F00,  // +0x00C4
    0x431868C8, 0x200160C8, 0xB570BD30, 0x1CC94D14,  // +0x00D4
    0x68EB0889, 0x26F00089, 0x60EB4333, 0x612B2300,  // +0x00E4
    0xE0174B15, 0x431C692C, 0x6814612C, 0x68EC6004,  // +0x00F4
    0xD4FC03E4, 0x0864692C, 0x612C0064, 0x062468EC,  // +0x0104
    0xD0040F24, 0x433068E8, 0x200160E8, 0x1D00BD70,  // +0x0114
    0x1F091D12, 0xD1E52900, 0xBD702000, 0x45670123,  // +0x0124
    0x40023C00, 0xCDEF89AB, 0x00005555, 0x40003000,  // +0x0134
    0x00000FFF, 0x0000AAAA, 0x00000201, 0x00000000,  // +0x0144
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x4000, 0x000000},
    {0x10000, 0x010000},
    {0x20000, 0x020000},
};

// Flash programming target configuration
const program_target_t _stm32f4xx_512_ = {
    0x2000002B,   // Init
    0x20000059,   // UnInit
    0x20000067,   // EraseChip
    0x20000093,   // EraseSector
    0x200000DF,   // ProgramPage
    {
        0x20000001,   // BKPT : 断点地址 (算法起始+1，Thumb模式)
        0x20000C00,   // RSB  : 静态数据基址
        0x20001000,   // RSP  : 栈指针地址
    },
    0x20000400,           // 编程缓冲区地址
    0x20000000,           // 算法代码起始地址
    sizeof(flash_code),   // 算法代码大小
    flash_code,           // 算法代码数据指针
    0x00000400,           // 编程缓冲区大小

    sector_info,                                    // 扇区信息指针
    sizeof(sector_info) / sizeof(sector_info[0]),   // 扇区数量
};