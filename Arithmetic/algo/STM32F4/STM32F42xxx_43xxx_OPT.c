#include "flash_blob.h"

/*
 * STM32F42xxx_43xxx_OPT SRAM布局 (基址0x20000000):
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
//   Init         @ +0x0005 (size: 42 bytes)
//   UnInit       @ +0x002F (size: 14 bytes)
//   SetRDP       @ +0x003D (size: 54 bytes)
//   EraseChip    @ +0x0073 (size: 48 bytes)
//   EraseSector  @ +0x00A3 (size: 4 bytes)
//   ProgramPage  @ +0x00A7 (size: 56 bytes)
//   Verify       @ +0x00DF (size: 40 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49404841, 0x49416081, 0x68C16081, 0x431122F0,  // +0x0004
    0x694060C1, 0xD4060680, 0x493D483E, 0x21066001,  // +0x0014
    0x493D6041, 0x20006081, 0x48374770, 0x22016941,  // +0x0024
    0x61414311, 0x47702000, 0x68C14833, 0x431122F0,  // +0x0034
    0x493660C1, 0x69416181, 0x021B23FF, 0x61414399,  // +0x0044
    0x23026941, 0x61414319, 0x060968C1, 0xD0040F09,  // +0x0054
    0x431168C1, 0x200160C1, 0x20004770, 0x48264770,  // +0x0064
    0x21F068C2, 0x60C2430A, 0x61824A28, 0x61424A28,  // +0x0074
    0x23026942, 0x6142431A, 0x061268C2, 0xD0040F12,  // +0x0084
    0x430A68C2, 0x200160C2, 0x20004770, 0x20004770,  // +0x0094
    0xB5104770, 0xCA064818, 0x23F068C4, 0x60C4431C,  // +0x00A4
    0x40224C1A, 0x4A1B6182, 0x1C894011, 0x68C16141,  // +0x00B4
    0xD4FC03C9, 0x060968C1, 0xD0040F09, 0x431968C1,  // +0x00C4
    0x200160C1, 0x2000BD10, 0xB570BD10, 0x68134E0A,  // +0x00D4
    0x68524C10, 0x40236975, 0x42AB4025, 0x4B0BD106,  // +0x00E4
    0x401A69B4, 0x42A2401C, 0x1C40D001, 0x1840BD70,  // +0x00F4
    0x0000BD70, 0x08192A3B, 0x40023C00, 0x4C5D6E7F,  // +0x0104
    0x00005555, 0x40003000, 0x00000FFF, 0x0FFF0000,  // +0x0114
    0x0FFFAAEC, 0xCFFFFFFC, 0x00000000,              // +0x0124
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0008, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f42xxx_43xxx_opt_ = {
    0x20000005,   // Init
    0x2000002F,   // UnInit
    0x20000073,   // EraseChip
    0x200000A3,   // EraseSector
    0x200000A7,   // ProgramPage
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