#include "flash_blob.h"

/*
 * STM32F10x_OPT SRAM布局 (基址0x20000000):
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
//   UnInit       @ +0x0033 (size: 22 bytes)
//   EraseChip    @ +0x0049 (size: 22 bytes)
//   EraseSector  @ +0x005F (size: 46 bytes)
//   ProgramPage  @ +0x008D (size: 80 bytes)
//   Verify       @ +0x00DD (size: 4 bytes)
//   SetRDP       @ +0x00E1 (size: 20 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4A514850, 0x60012100, 0x60414950, 0x60816042,  // +0x0004
    0x69C06082, 0xD4080741, 0xF245484D, 0x60025255,  // +0x0014
    0x60412106, 0x73FFF640, 0x20006083, 0x48494770,  // +0x0024
    0xF4216801, 0x60017180, 0xF0426802, 0x60020280,  // +0x0034
    0x47702000, 0xF000B530, 0xF645F866, 0xF00024A5,  // +0x0044
    0xD000F84F, 0x2000E084, 0x483FBD30, 0x68414A3C,  // +0x0054
    0x0120F041, 0x68416041, 0x0140F041, 0xE0026041,  // +0x0064
    0x21AAF64A, 0x68036011, 0xD4F907D9, 0xF0226842,  // +0x0074
    0x60420220, 0x47702000, 0x1C49B570, 0x00490849,  // +0x0084
    0x4E2F4B31, 0x1C80E002, 0x1E891C92, 0x685CB1D1,  // +0x0094
    0x0410F044, 0x8815605C, 0xE0028005, 0x24AAF64A,  // +0x00A4
    0x681D6034, 0xD4F907EC, 0xF025685D, 0x605D0510,  // +0x00B4
    0xF014681C, 0xD0E60F14, 0xF0406818, 0x60180014,  // +0x00C4
    0xBD702001, 0xBD702000, 0x47701808, 0xF000B530,  // +0x00D4
    0x2400F81A, 0xF804F000, 0xE039D000, 0xBD302000,  // +0x00E4
    0xF0436843, 0x60430310, 0x801C4B18, 0x600AE000,  // +0x00F4
    0x07EB6805, 0x6841D4FB, 0x0110F021, 0x68026041,  // +0x0104
    0x0F14F012, 0x48104770, 0xF0416841, 0x60410120,  // +0x0114
    0x22AAF64A, 0xF0416841, 0x60410140, 0xE0004908,  // +0x0124
    0x6803600A, 0xD4FB07DC, 0xF0236843, 0x60430320,  // +0x0134
    0x00004770, 0x40022000, 0xCDEF89AB, 0x45670123,  // +0x0144
    0x40003000, 0x40022010, 0x4002200C, 0x1FFFF800,  // +0x0154
    0xF0416801, 0x60010114, 0xBD302001, 0x00000000,  // +0x0164
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0010, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_opt_ = {
    0x20000005,   // Init
    0x20000033,   // UnInit
    0x20000049,   // EraseChip
    0x2000005F,   // EraseSector
    0x2000008D,   // ProgramPage
    0x200000E1,   // SetRDP
    0x200000DD,   // Verify
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