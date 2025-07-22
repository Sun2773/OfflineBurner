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
//   Init         @ +0x0005 (size: 58 bytes)
//   UnInit       @ +0x003F (size: 28 bytes)
//   EraseChip    @ +0x005B (size: 136 bytes)
//   EraseSector  @ +0x00E3 (size: 62 bytes)
//   BlankCheck   @ +0x0121 (size: 6 bytes)
//   ProgramPage  @ +0x0127 (size: 100 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4603B510, 0x4C602000, 0x48606020, 0x48606060,  // +0x0004
    0x485E6060, 0x485E60A0, 0x462060A0, 0xF00069C0,  // +0x0014
    0xB9400004, 0x5055F245, 0x60204C5A, 0x60602006,  // +0x0024
    0x70FFF640, 0x200060A0, 0x4601BD10, 0x69004852,  // +0x0034
    0x7080F420, 0x61104A50, 0x69004610, 0x0080F040,  // +0x0044
    0x20006110, 0x484C4770, 0xF0406900, 0x494A0020,  // +0x0054
    0x46086108, 0xF0406900, 0x61080040, 0xF64AE003,  // +0x0064
    0x494820AA, 0x48446008, 0xF00068C0, 0x28000001,  // +0x0074
    0x4841D1F5, 0xF0206900, 0x493F0020, 0x46086108,  // +0x0084
    0xF0406900, 0x61080010, 0x20A5F645, 0x8008493E,  // +0x0094
    0xF64AE003, 0x493B20AA, 0x48376008, 0xF00068C0,  // +0x00A4
    0x28000001, 0x4834D1F5, 0xF0206900, 0x49320010,  // +0x00B4
    0x46086108, 0xF00068C0, 0xB1300014, 0x68C04608,  // +0x00C4
    0x0014F040, 0x200160C8, 0x20004770, 0x4601E7FC,  // +0x00D4
    0x69004829, 0x0020F040, 0x61104A27, 0x69004610,  // +0x00E4
    0x0040F040, 0xE0036110, 0x20AAF64A, 0x60104A25,  // +0x00F4
    0x68C04821, 0x0001F000, 0xD1F52800, 0x6900481E,  // +0x0104
    0x0020F020, 0x61104A1C, 0x47702000, 0x20014603,  // +0x0114
    0xB5104770, 0x1C484603, 0x0101F020, 0x4816E027,  // +0x0124
    0xF0406900, 0x4C140010, 0x88106120, 0xE0038018,  // +0x0134
    0x20AAF64A, 0x60204C13, 0x68C0480F, 0x0001F000,  // +0x0144
    0xD1F52800, 0x6900480C, 0x0010F020, 0x61204C0A,  // +0x0154
    0x68C04620, 0x0014F000, 0x4620B130, 0xF04068C0,  // +0x0164
    0x60E00014, 0xBD102001, 0x1C921C9B, 0x29001E89,  // +0x0174
    0x2000D1D5, 0x0000E7F7, 0x40022000, 0x45670123,  // +0x0184
    0xCDEF89AB, 0x40003000, 0x1FFFF800, 0x00000000,  // +0x0194
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0010, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_opt_ = {
    0x20000005,   // Init
    0x2000003F,   // UnInit
    0x2000005B,   // EraseChip
    0x200000E3,   // EraseSector
    0x20000127,   // ProgramPage
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