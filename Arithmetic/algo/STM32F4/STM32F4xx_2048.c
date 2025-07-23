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
//   GetSecNum    @ +0x0005 (size: 36 bytes)
//   Init         @ +0x0029 (size: 50 bytes)
//   UnInit       @ +0x005B (size: 14 bytes)
//   EraseChip    @ +0x0069 (size: 62 bytes)
//   EraseSector  @ +0x00A7 (size: 88 bytes)
//   ProgramPage  @ +0x00FF (size: 84 bytes)
//   Verify       @ +0x0153 (size: 62 bytes)
//   SetRDP       @ +0x01AD (size: 4 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0xF3C14601, 0x28203007, 0x0940D302, 0xE0041D00,  // +0x0004
    0xBF262810, 0x1CC00900, 0x02C90880, 0xF040BF48,  // +0x0014
    0x47700010, 0x495A4859, 0x60414A5A, 0x21006042,  // +0x0024
    0x68C26001, 0x02F0F042, 0x694060C2, 0xD4080681,  // +0x0034
    0xF2454855, 0x60025255, 0x60412106, 0x73FFF640,  // +0x0044
    0x20006083, 0x48514770, 0xF0416801, 0x60014100,  // +0x0054
    0x47702000, 0x4A4C484E, 0xF0416841, 0x60410104,  // +0x0064
    0xF4416841, 0x60414100, 0xF4416841, 0x60413180,  // +0x0074
    0xF64AE002, 0x601121AA, 0x03D96803, 0x6842D4F9,  // +0x0084
    0x0204F022, 0x68416042, 0x4100F421, 0x20006041,  // +0x0094
    0xB5804770, 0xFFACF7FF, 0x680A493D, 0x02F0F042,  // +0x00A4
    0x2302600A, 0x00C0604B, 0xF000684A, 0x431000F8,  // +0x00B4
    0x4A356048, 0xF4406848, 0x60483080, 0xF64AE002,  // +0x00C4
    0x601020AA, 0x03D8680B, 0x684AD4F9, 0x0202F022,  // +0x00D4
    0x6808604A, 0x0FF0F010, 0x6808D005, 0x00F0F040,  // +0x00E4
    0x20016008, 0x2000BD02, 0xB570BD02, 0x68234C28,  // +0x00F4
    0xF0431CC9, 0x088903F0, 0x25006023, 0x60650089,  // +0x0104
    0x1D00E002, 0x1F091D12, 0x6865B1B9, 0x2301F240,  // +0x0114
    0x6065431D, 0x60066816, 0x03DD6823, 0x6863D4FC,  // +0x0124
    0x005B085B, 0x68256063, 0x0FF0F015, 0x6820D0E9,  // +0x0134
    0x00F0F040, 0x20016020, 0x2000BD70, 0xB5E0BD70,  // +0x0144
    0xF04F0003, 0xD00F35FF, 0xE00B2600, 0x40455D98,  // +0x0154
    0x086F2008, 0x463D07ED, 0x4F0EBF44, 0x1E40407D,  // +0x0164
    0x1C76D1F7, 0xD3F1428E, 0xF04F7812, 0x404530FF,  // +0x0174
    0xD00142AA, 0xBDE04618, 0xBDE018C8, 0x40023C00,  // +0x0184
    0x45670123, 0xCDEF89AB, 0x40003000, 0x40023C10,  // +0x0194
    0x40023C0C, 0xEDB88320, 0x47702000, 0x00000000,  // +0x01A4
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
    0x20000029,   // Init
    0x2000005B,   // UnInit
    0x20000069,   // EraseChip
    0x200000A7,   // EraseSector
    0x200000FF,   // ProgramPage
    0x200001AD,   // SetRDP
    0x20000153,   // Verify
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