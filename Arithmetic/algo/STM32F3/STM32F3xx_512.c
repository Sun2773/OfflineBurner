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
//   Init         @ +0x0005 (size: 50 bytes)
//   UnInit       @ +0x0037 (size: 14 bytes)
//   EraseChip    @ +0x0045 (size: 46 bytes)
//   EraseSector  @ +0x0073 (size: 48 bytes)
//   ProgramPage  @ +0x00A3 (size: 70 bytes)
//   Verify       @ +0x00E9 (size: 62 bytes)
//   SetRDP       @ +0x0145 (size: 4 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49494848, 0x60414A49, 0x21006042, 0x68C26001,  // +0x0004
    0x0214F042, 0x69C060C2, 0xD40805C1, 0xF2454844,  // +0x0014
    0x60025255, 0x60412106, 0x73FFF640, 0x20006083,  // +0x0024
    0x48404770, 0xF0416801, 0x60010180, 0x47702000,  // +0x0034
    0x4A3B483D, 0xF0416841, 0x60410104, 0xF0416841,  // +0x0044
    0x60410140, 0xF64AE002, 0x601121AA, 0x07D96803,  // +0x0054
    0x6842D4F9, 0x0204F022, 0x20006042, 0x49324770,  // +0x0064
    0xF042684A, 0x604A0202, 0x4A2D6088, 0xF0406848,  // +0x0074
    0x60480040, 0xF64AE002, 0x601020AA, 0x07D8680B,  // +0x0084
    0x684AD4F9, 0x0202F022, 0x2000604A, 0xB5304770,  // +0x0094
    0x08491C49, 0x4B240049, 0x1C80E002, 0x1E891C92,  // +0x00A4
    0x685CB1B1, 0x0401F044, 0x8815605C, 0x681C8005,  // +0x00B4
    0xD4FC07E5, 0x0864685C, 0x605C0064, 0xF015681D,  // +0x00C4
    0xD0EA0F14, 0xF0406818, 0x60180014, 0xBD302001,  // +0x00D4
    0xBD302000, 0x0003B5E0, 0x35FFF04F, 0x2600D00F,  // +0x00E4
    0x5D98E00B, 0x20084045, 0x07ED086F, 0xBF44463D,  // +0x00F4
    0x407D4F0E, 0xD1F71E40, 0x428E1C76, 0x6812D3F1,  // +0x0104
    0x30FFF04F, 0x42AA4045, 0x4618D001, 0x18C8BDE0,  // +0x0114
    0x0000BDE0, 0x40022000, 0x45670123, 0xCDEF89AB,  // +0x0124
    0x40003000, 0x40022010, 0x4002200C, 0xEDB88320,  // +0x0134
    0x47702000, 0x00000000,                          // +0x0144
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f3xx_512_ = {
    0x20000005,   // Init
    0x20000037,   // UnInit
    0x20000045,   // EraseChip
    0x20000073,   // EraseSector
    0x200000A3,   // ProgramPage
    0x20000145,   // SetRDP
    0x200000E9,   // Verify
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