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
//   Init         @ +0x0005 (size: 54 bytes)
//   UnInit       @ +0x003B (size: 22 bytes)
//   EraseChip    @ +0x0051 (size: 82 bytes)
//   EraseSector  @ +0x00A3 (size: 100 bytes)
//   ProgramPage  @ +0x0107 (size: 134 bytes)
//   Verify       @ +0x018D (size: 62 bytes)
//   SetRDP       @ +0x01ED (size: 4 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49710CC0, 0x600804C0, 0x48702200, 0x60024970,  // +0x0004
    0x4A706041, 0x64416042, 0x69C06442, 0xD4080741,  // +0x0014
    0xF245486D, 0x60025255, 0x60412106, 0x73FFF640,  // +0x0024
    0x20006083, 0x48694770, 0xF0416801, 0x60010180,  // +0x0034
    0xF0426C02, 0x64020280, 0x47702000, 0x4864B510,  // +0x0044
    0xF0416841, 0x60410104, 0x22AAF64A, 0xF0416841,  // +0x0054
    0x60410140, 0xE000495C, 0x6803600A, 0xD4FB07DC,  // +0x0064
    0xF0236843, 0x60430304, 0xF0436C43, 0x64430304,  // +0x0074
    0xF0436C43, 0x64430340, 0x600AE000, 0x07DC6C03,  // +0x0084
    0x6C41D4FB, 0x0104F021, 0x20006441, 0xB530BD10,  // +0x0094
    0x4A4D4C49, 0x494E6825, 0x2500F505, 0xF64A42A8,  // +0x00A4
    0xD21223AA, 0xF044684C, 0x604C0402, 0x68486088,  // +0x00B4
    0x0040F040, 0xE0006048, 0x68086013, 0xD4FB07C4,  // +0x00C4
    0xF0206848, 0x60480002, 0x6C4CE011, 0x0402F044,  // +0x00D4
    0x6488644C, 0xF0406C48, 0x64480040, 0x6013E000,  // +0x00E4
    0x07C46C08, 0x6C48D4FB, 0x0002F020, 0x20006448,  // +0x00F4
    0xB530BD30, 0x4B364C30, 0x1C496825, 0x00490849,  // +0x0104
    0x2500F505, 0xD21D42A8, 0x1C80E002, 0x1E891C92,  // +0x0114
    0x685CB381, 0x0401F044, 0x8815605C, 0x681C8005,  // +0x0124
    0xD4FC07E5, 0x0864685C, 0x605C0064, 0xF015681D,  // +0x0134
    0xD0EA0F14, 0xF0406818, 0x60180014, 0x1C80E018,  // +0x0144
    0x1E891C92, 0x6C5CB1B1, 0x0401F044, 0x8815645C,  // +0x0154
    0x6C1C8005, 0xD4FC07E5, 0x08646C5C, 0x645C0064,  // +0x0164
    0xF0146C1C, 0xD0EA0F14, 0xF0406C18, 0x64180014,  // +0x0174
    0xBD302001, 0xBD302000, 0x0003B5E0, 0x35FFF04F,  // +0x0184
    0x2600D00F, 0x5D98E00B, 0x20084045, 0x07ED086F,  // +0x0194
    0xBF44463D, 0x407D4F0F, 0xD1F71E40, 0x428E1C76,  // +0x01A4
    0x6812D3F1, 0x30FFF04F, 0x42AA4045, 0x4618D001,  // +0x01B4
    0x18C8BDE0, 0x0000BDE0, 0x00000400, 0x40022000,  // +0x01C4
    0x45670123, 0xCDEF89AB, 0x40003000, 0x40022010,  // +0x01D4
    0x4002200C, 0xEDB88320, 0x47702000, 0x00000000,  // +0x01E4
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_1024_ = {
    0x20000005,   // Init
    0x2000003B,   // UnInit
    0x20000051,   // EraseChip
    0x200000A3,   // EraseSector
    0x20000107,   // ProgramPage
    0x200001ED,   // SetRDP
    0x2000018D,   // Verify
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