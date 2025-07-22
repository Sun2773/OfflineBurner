#include "flash_blob.h"

/*
 * STM32F0xx_OPT SRAM布局 (基址0x20000000):
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
//   UnInit       @ +0x0037 (size: 22 bytes)
//   EraseChip    @ +0x004D (size: 100 bytes)
//   EraseSector  @ +0x00B1 (size: 68 bytes)
//   BlankCheck   @ +0x00F5 (size: 4 bytes)
//   ProgramPage  @ +0x00F9 (size: 86 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4A524853, 0x49536042, 0x60826041, 0x21006081,  // +0x0004
    0x68C16001, 0x43112214, 0x69C060C1, 0xD4060740,  // +0x0014
    0x494D484E, 0x21066001, 0x494D6041, 0x20006081,  // +0x0024
    0x48474770, 0x22806901, 0x61014311, 0x15826901,  // +0x0034
    0x61014391, 0x47702000, 0x4841B570, 0x231468C1,  // +0x0044
    0x60C14319, 0x24206901, 0x61014321, 0x22406901,  // +0x0054
    0x61014311, 0x4A3D493F, 0x6011E000, 0x07ED68C5,  // +0x0064
    0x6905D1FB, 0x610543A5, 0x24106905, 0x61054325,  // +0x0074
    0x4E394D35, 0x80353555, 0x6011E000, 0x07ED68C5,  // +0x0084
    0x6901D1FB, 0x610143A1, 0x421968C1, 0x68C1D004,  // +0x0094
    0x60C14319, 0xBD702001, 0xBD702000, 0x4828B530,  // +0x00A4
    0x241468C1, 0x60C14321, 0x25206901, 0x61014329,  // +0x00B4
    0x22406901, 0x61014311, 0x4A244926, 0x6011E000,  // +0x00C4
    0x07DB68C3, 0x6901D1FB, 0x610143A9, 0x422168C1,  // +0x00D4
    0x68C1D004, 0x60C14321, 0xBD302001, 0xBD302000,  // +0x00E4
    0x47702001, 0x4D16B5F0, 0x08491C49, 0x004968EB,  // +0x00F4
    0x43232404, 0x261060EB, 0xE01A4B16, 0x4334692C,  // +0x0104
    0x8814612C, 0x4C118004, 0x6023E000, 0x07FF68EF,  // +0x0114
    0x692CD1FB, 0x612C43B4, 0x271468EC, 0xD005423C,  // +0x0124
    0x211468E8, 0x60E84308, 0xBDF02001, 0x1E891C80,  // +0x0134
    0x29001C92, 0x2000D1E2, 0x0000BDF0, 0x45670123,  // +0x0144
    0x40022000, 0xCDEF89AB, 0x00005555, 0x40003000,  // +0x0154
    0x00000FFF, 0x0000AAAA, 0x1FFFF800, 0x00000000,  // +0x0164
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0010, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f0xx_opt_ = {
    0x20000005,   // Init
    0x20000037,   // UnInit
    0x2000004D,   // EraseChip
    0x200000B1,   // EraseSector
    0x200000F9,   // ProgramPage
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