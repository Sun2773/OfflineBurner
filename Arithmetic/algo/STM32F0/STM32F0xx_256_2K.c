#include "flash_blob.h"

/*
 * STM32F0xx_256_2K SRAM布局 (基址0x20000000):
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
//   EraseChip    @ +0x0041 (size: 68 bytes)
//   EraseSector  @ +0x0085 (size: 66 bytes)
//   ProgramPage  @ +0x00C7 (size: 86 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49454846, 0x49466041, 0x21006041, 0x68C16001,  // +0x0004
    0x43112214, 0x69C060C1, 0xD4060740, 0x49414842,  // +0x0014
    0x21066001, 0x49416041, 0x20006081, 0x483B4770,  // +0x0024
    0x22806901, 0x61014311, 0x47702000, 0x4837B530,  // +0x0034
    0x241468C1, 0x60C14321, 0x25046901, 0x61014329,  // +0x0044
    0x22406901, 0x61014311, 0x4A334935, 0x6011E000,  // +0x0054
    0x07DB68C3, 0x6901D1FB, 0x610143A9, 0x422168C1,  // +0x0064
    0x68C1D004, 0x60C14321, 0xBD302001, 0xBD302000,  // +0x0074
    0x4926B530, 0x231468CA, 0x60CA431A, 0x2402690A,  // +0x0084
    0x610A4322, 0x69086148, 0x43102240, 0x48246108,  // +0x0094
    0xE0004A21, 0x68CD6010, 0xD1FB07ED, 0x43A06908,  // +0x00A4
    0x68C86108, 0xD0034018, 0x431868C8, 0x200160C8,  // +0x00B4
    0xB5F0BD30, 0x1C494D15, 0x68EB0849, 0x24040049,  // +0x00C4
    0x60EB4323, 0x4C162714, 0x692BE01A, 0x43332601,  // +0x00D4
    0x8813612B, 0x4B108003, 0x601CE000, 0x07F668EE,  // +0x00E4
    0x692BD1FB, 0x005B085B, 0x68EB612B, 0xD004423B,  // +0x00F4
    0x433868E8, 0x200160E8, 0x1C80BDF0, 0x1C921E89,  // +0x0104
    0xD1E22900, 0xBDF02000, 0x45670123, 0x40022000,  // +0x0114
    0xCDEF89AB, 0x00005555, 0x40003000, 0x00000FFF,  // +0x0124
    0x0000AAAA, 0x00000000,                          // +0x0134
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f0xx_256_2k_ = {
    0x20000005,   // Init
    0x20000033,   // UnInit
    0x20000041,   // EraseChip
    0x20000085,   // EraseSector
    0x200000C7,   // ProgramPage
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