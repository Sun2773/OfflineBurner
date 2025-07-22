#include "flash_blob.h"

/*
 * STM32F4xx_2048 SRAM布局 (基址0x20000000):
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
//   Init         @ +0x002B (size: 46 bytes)
//   UnInit       @ +0x0059 (size: 14 bytes)
//   EraseChip    @ +0x0067 (size: 58 bytes)
//   EraseSector  @ +0x00A1 (size: 76 bytes)
//   ProgramPage  @ +0x00ED (size: 82 bytes)
//   Verify       @ +0x013F (size: 50 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x03004601, 0x28200E00, 0x0940D302, 0xE0051D00,  // +0x0004
    0xD3022810, 0x1CC00900, 0x0880E000, 0xD50102C9,  // +0x0014
    0x43082110, 0x48524770, 0x60414950, 0x60414951,  // +0x0024
    0x60012100, 0x22F068C1, 0x60C14311, 0x06806940,  // +0x0034
    0x484ED406, 0x6001494C, 0x60412106, 0x6081494C,  // +0x0044
    0x47702000, 0x69014846, 0x43110542, 0x20006101,  // +0x0054
    0xB5304770, 0x69014842, 0x43212404, 0x69016101,  // +0x0064
    0x43290365, 0x69016101, 0x431103A2, 0x49416101,  // +0x0074
    0xE0004A3E, 0x68C36011, 0xD4FB03DB, 0x43A16901,  // +0x0084
    0x69016101, 0x610143A9, 0xBD302000, 0xF7FFB530,  // +0x0094
    0x4933FFAF, 0x23F068CA, 0x60CA431A, 0x610C2402,  // +0x00A4
    0x06C0690A, 0x43020E00, 0x6908610A, 0x431003E2,  // +0x00B4
    0x48306108, 0xE0004A2D, 0x68CD6010, 0xD4FB03ED,  // +0x00C4
    0x43A06908, 0x68C86108, 0x0F000600, 0x68C8D003,  // +0x00D4
    0x60C84318, 0xBD302001, 0x4D21B570, 0x08891CC9,  // +0x00E4
    0x008968EB, 0x433326F0, 0x230060EB, 0x4B22612B,  // +0x00F4
    0x692CE017, 0x612C431C, 0x60046814, 0x03E468EC,  // +0x0104
    0x692CD4FC, 0x00640864, 0x68EC612C, 0x0F240624,  // +0x0114
    0x68E8D004, 0x60E84330, 0xBD702001, 0x1F091D00,  // +0x0124
    0x29001D12, 0x2000D1E5, 0xB570BD70, 0x2A0043C0,  // +0x0134
    0x2400D012, 0xE00D4D11, 0x40585D13, 0x07C62300,  // +0x0144
    0x0840D002, 0xE0004068, 0x1C5B0840, 0x2B08B2DB,  // +0x0154
    0x1C64D3F5, 0xD3EF428C, 0xBD7043C0, 0x45670123,  // +0x0164
    0x40023C00, 0xCDEF89AB, 0x00005555, 0x40003000,  // +0x0174
    0x00000FFF, 0x0000AAAA, 0x00000201, 0xEDB88320,  // +0x0184
    0x00000000                                       // +0x0194
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x4000, 0x000000},
    {0x10000, 0x010000},
    {0x20000, 0x020000},
    {0x4000, 0x100000},
    {0x10000, 0x110000},
    {0x20000, 0x120000},
};

// Flash programming target configuration
const program_target_t _stm32f4xx_2048_ = {
    0x2000002B,   // Init
    0x20000059,   // UnInit
    0x20000067,   // EraseChip
    0x200000A1,   // EraseSector
    0x200000ED,   // ProgramPage
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