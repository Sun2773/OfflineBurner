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
//   EraseSector  @ +0x0085 (size: 68 bytes)
//   ProgramPage  @ +0x00C9 (size: 86 bytes)
//   Verify       @ +0x011F (size: 68 bytes)
//   SetRDP       @ +0x018D (size: 4 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x49584857, 0x49586041, 0x21006041, 0x68C16001,  // +0x0004
    0x430A2214, 0x69C060C2, 0xD4060740, 0x49544853,  // +0x0014
    0x21066001, 0x49536041, 0x20006081, 0x48524770,  // +0x0024
    0x22806801, 0x6002430A, 0x47702000, 0x494F2014,  // +0x0034
    0x4302680A, 0x684A600A, 0x43132304, 0x684A604B,  // +0x0044
    0x43132340, 0x4B45604B, 0x4A49E001, 0x680A601A,  // +0x0054
    0xD4FA07D2, 0x2304684A, 0x604A439A, 0x4002680A,  // +0x0064
    0x680AD004, 0x60084310, 0x47702001, 0x47702000,  // +0x0074
    0x2114B510, 0x68134A3D, 0x6013430B, 0x24026853,  // +0x0084
    0x6054431C, 0x68506090, 0x43032340, 0x4B336053,  // +0x0094
    0x4837E001, 0x68106018, 0xD4FA07C0, 0x23026850,  // +0x00A4
    0x60504398, 0x40086810, 0x6810D003, 0x60114301,  // +0x00B4
    0xBD102001, 0x1C49B5F0, 0x43992301, 0x681C4B2B,  // +0x00C4
    0x43252504, 0x2414601D, 0xE0024F24, 0x1C921C80,  // +0x00D4
    0x29001E89, 0x2501D017, 0x432E685E, 0x8816605E,  // +0x00E4
    0xE0018006, 0x603E4E22, 0x402E681E, 0x685DD1FA,  // +0x00F4
    0x43B52601, 0x681D605D, 0xD0E74025, 0x43046818,  // +0x0104
    0x2001601C, 0x2000BDF0, 0xB5F8BDF0, 0x93000003,  // +0x0114
    0x43E42400, 0x28000026, 0x2500D011, 0x0006E00D,  // +0x0124
    0x5D58E008, 0x00064070, 0x08702708, 0xD5F607F6,  // +0x0134
    0x40464E10, 0xD1F81E7F, 0x428D1C6D, 0x6810D3F1,  // +0x0144
    0x42A04074, 0x0018D001, 0x1858BDF2, 0x0000BDF2,  // +0x0154
    0x40022000, 0x45670123, 0xCDEF89AB, 0x40003000,  // +0x0164
    0x00005555, 0x00000FFF, 0x40022010, 0x4002200C,  // +0x0174
    0x0000AAAA, 0xEDB88320, 0x47702000, 0x00000000,  // +0x0184
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
    0x200000C9,   // ProgramPage
    0x2000018D,   // SetRDP
    0x2000011F,   // Verify
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