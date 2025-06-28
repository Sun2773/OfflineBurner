#include "flash_blob.h"

/*
 * STM32F4xx_1024 SRAM布局 (基址0x20000000):
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
//   GetSecNum    @ +0x0005 (size: 28 bytes)
//   Init         @ +0x0021 (size: 46 bytes)
//   UnInit       @ +0x004F (size: 14 bytes)
//   EraseChip    @ +0x005D (size: 44 bytes)
//   EraseSector  @ +0x0089 (size: 76 bytes)
//   ProgramPage  @ +0x00D5 (size: 82 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x0E000300, 0xD3022820, 0x1D000940, 0x28104770,  // +0x0004
    0x0900D302, 0x47701CC0, 0x47700880, 0x49414842,  // +0x0014
    0x49426041, 0x21006041, 0x68C16001, 0x431122F0,  // +0x0024
    0x694060C1, 0xD4060680, 0x493D483E, 0x21066001,  // +0x0034
    0x493D6041, 0x20006081, 0x48374770, 0x05426901,  // +0x0044
    0x61014311, 0x47702000, 0x4833B510, 0x24046901,  // +0x0054
    0x61014321, 0x03A26901, 0x61014311, 0x4A314933,  // +0x0064
    0x6011E000, 0x03DB68C3, 0x6901D4FB, 0x610143A1,  // +0x0074
    0xBD102000, 0xF7FFB530, 0x4927FFBB, 0x23F068CA,  // +0x0084
    0x60CA431A, 0x610C2402, 0x0700690A, 0x43020E40,  // +0x0094
    0x6908610A, 0x431003E2, 0x48246108, 0xE0004A21,  // +0x00A4
    0x68CD6010, 0xD4FB03ED, 0x43A06908, 0x68C86108,  // +0x00B4
    0x0F000600, 0x68C8D003, 0x60C84318, 0xBD302001,  // +0x00C4
    0x4D15B570, 0x08891CC9, 0x008968EB, 0x433326F0,  // +0x00D4
    0x230060EB, 0x4B16612B, 0x692CE017, 0x612C431C,  // +0x00E4
    0x60046814, 0x03E468EC, 0x692CD4FC, 0x00640864,  // +0x00F4
    0x68EC612C, 0x0F240624, 0x68E8D004, 0x60E84330,  // +0x0104
    0xBD702001, 0x1D121D00, 0x29001F09, 0x2000D1E5,  // +0x0114
    0x0000BD70, 0x45670123, 0x40023C00, 0xCDEF89AB,  // +0x0124
    0x00005555, 0x40003000, 0x00000FFF, 0x0000AAAA,  // +0x0134
    0x00000201, 0x00000000,                          // +0x0144
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x4000, 0x000000},
    {0x10000, 0x010000},
    {0x20000, 0x020000},
};

// Flash programming target configuration
const program_target_t _stm32f4xx_1024_ = {
    0x20000021,   // Init
    0x2000004F,   // UnInit
    0x2000005D,   // EraseChip
    0x20000089,   // EraseSector
    0x200000D5,   // ProgramPage
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