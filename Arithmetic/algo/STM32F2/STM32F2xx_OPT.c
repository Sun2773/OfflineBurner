#include "flash_blob.h"

/*
 * STM32F2xx_OPT SRAM布局 (基址0x20000000):
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
//   EraseChip    @ +0x0041 (size: 36 bytes)
//   EraseSector  @ +0x0065 (size: 4 bytes)
//   ProgramPage  @ +0x0069 (size: 66 bytes)
//   Verify       @ +0x00AB (size: 4 bytes)
//   SetRDP       @ +0x00AF (size: 40 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49354834, 0x60014A35, 0x68436002, 0x03F0F043,  // +0x0004
    0x68C06043, 0xD4080681, 0xF2454831, 0x60025255,  // +0x0014
    0x60412106, 0x73FFF640, 0x20006083, 0x482D4770,  // +0x0024
    0xF0416801, 0x60010101, 0x47702000, 0x4A2B482A,  // +0x0034
    0xF0416801, 0x600101F0, 0x68816082, 0x0102F041,  // +0x0044
    0x68026081, 0x0FF0F012, 0xE04BD000, 0x47702000,  // +0x0054
    0x47702000, 0x8813B510, 0x491F8910, 0x680A4C20,  // +0x0064
    0x4400EA04, 0x70FCF64F, 0xF0424003, 0x432302F0,  // +0x0074
    0xF043600A, 0x608B0302, 0x03C26808, 0x6808D4FC,  // +0x0084
    0x0FF0F010, 0x6808D005, 0x00F0F040, 0x20016008,  // +0x0094
    0x2000BD10, 0x1808BD10, 0x480F4770, 0xF0416801,  // +0x00A4
    0x600101F0, 0xF4226882, 0x6082427F, 0xF0416881,  // +0x00B4
    0x60810102, 0xF0126802, 0xD0000FF0, 0x2000E012,  // +0x00C4
    0x00004770, 0x40023C08, 0x08192A3B, 0x4C5D6E7F,  // +0x00D4
    0x40003000, 0x40023C14, 0x40023C0C, 0x0FFFAAEC,  // +0x00E4
    0x0FFF0000, 0xF0416801, 0x600101F0, 0x47702001,  // +0x00F4
    0x00000000                                       // +0x0104
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0010, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f2xx_opt_ = {
    0x20000005,   // Init
    0x20000033,   // UnInit
    0x20000041,   // EraseChip
    0x20000065,   // EraseSector
    0x20000069,   // ProgramPage
    0x200000AF,   // SetRDP
    0x200000AB,   // Verify
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