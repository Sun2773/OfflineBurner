#include "flash_blob.h"

/*
 * STM32F3xx_512 SRAM布局 (基址0x20000000):
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
//   Init         @ +0x0005 (size: 46 bytes)
//   UnInit       @ +0x0033 (size: 14 bytes)
//   EraseChip    @ +0x0041 (size: 44 bytes)
//   EraseSector  @ +0x006D (size: 46 bytes)
//   ProgramPage  @ +0x009B (size: 70 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49364837, 0x49376041, 0x21006041, 0x68C16001,  // +0x0004
    0x43112214, 0x69C060C1, 0xD40605C0, 0x49324833,  // +0x0014
    0x21066001, 0x49326041, 0x20006081, 0x482C4770,  // +0x0024
    0x22806901, 0x61014311, 0x47702000, 0x4828B510,  // +0x0034
    0x24046901, 0x61014321, 0x22406901, 0x61014311,  // +0x0044
    0x4A264928, 0x6011E000, 0x07DB68C3, 0x6901D1FB,  // +0x0054
    0x610143A1, 0xBD102000, 0x491DB510, 0x2402690A,  // +0x0064
    0x610A4322, 0x69086148, 0x43102240, 0x481D6108,  // +0x0074
    0xE0004A1A, 0x68CB6010, 0xD1FB07DB, 0x43A06908,  // +0x0084
    0x20006108, 0xB570BD10, 0x08491C49, 0x26140049,  // +0x0094
    0x23014D0F, 0x692CE016, 0x612C431C, 0x80048814,  // +0x00A4
    0x07E468EC, 0x692CD1FC, 0x00640864, 0x68EC612C,  // +0x00B4
    0xD0044234, 0x433068E8, 0x200160E8, 0x1C80BD70,  // +0x00C4
    0x1C921E89, 0xD1E62900, 0xBD702000, 0x45670123,  // +0x00D4
    0x40022000, 0xCDEF89AB, 0x00005555, 0x40003000,  // +0x00E4
    0x00000FFF, 0x0000AAAA, 0x00000000,              // +0x00F4
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f3xx_512_ = {
    0x20000005,   // Init
    0x20000033,   // UnInit
    0x20000041,   // EraseChip
    0x2000006D,   // EraseSector
    0x2000009B,   // ProgramPage
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