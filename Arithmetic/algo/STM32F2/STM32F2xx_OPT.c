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
//   GetSecNum    @ +0x0005 (size: 28 bytes)
//   Init         @ +0x0021 (size: 42 bytes)
//   UnInit       @ +0x004B (size: 14 bytes)
//   EraseChip    @ +0x0059 (size: 44 bytes)
//   EraseSector  @ +0x0085 (size: 4 bytes)
//   ProgramPage  @ +0x0089 (size: 58 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x0E000300, 0xD3022820, 0x1D000940, 0x28104770,  // +0x0004
    0x0900D302, 0x47701CC0, 0x47700880, 0x49284829,  // +0x0014
    0x49296081, 0x68C16081, 0x431122F0, 0x694060C1,  // +0x0024
    0xD4060680, 0x49254826, 0x21066001, 0x49256041,  // +0x0034
    0x20006081, 0x481F4770, 0x22016941, 0x61414311,  // +0x0044
    0x47702000, 0x68C2481B, 0x430A21F0, 0x4A1E60C2,  // +0x0054
    0x69426142, 0x431A2302, 0x68C26142, 0x0F120612,  // +0x0064
    0x68C2D004, 0x60C2430A, 0x47702001, 0x47702000,  // +0x0074
    0x47702000, 0x88108911, 0x09090509, 0x00800880,  // +0x0084
    0x480C4301, 0x22F068C3, 0x60C34313, 0x43192302,  // +0x0094
    0x68C16141, 0xD4FC03C9, 0x060968C1, 0xD0040F09,  // +0x00A4
    0x431168C1, 0x200160C1, 0x20004770, 0x00004770,  // +0x00B4
    0x08192A3B, 0x40023C00, 0x4C5D6E7F, 0x00005555,  // +0x00C4
    0x40003000, 0x00000FFF, 0x0FFFAAEC, 0x00000000,  // +0x00D4
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0010, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f2xx_opt_ = {
    0x20000021,   // Init
    0x2000004B,   // UnInit
    0x20000059,   // EraseChip
    0x20000085,   // EraseSector
    0x20000089,   // ProgramPage
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