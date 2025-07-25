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
//   EraseChip    @ +0x004D (size: 96 bytes)
//   EraseSector  @ +0x00AD (size: 68 bytes)
//   ProgramPage  @ +0x00F1 (size: 86 bytes)
//   Verify       @ +0x0147 (size: 4 bytes)
//   SetRDP       @ +0x014B (size: 96 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x496A4869, 0x4A6A6041, 0x60816042, 0x21006082,  // +0x0004
    0x68C16001, 0x430A2214, 0x69C060C2, 0xD4060740,  // +0x0014
    0x49654864, 0x21066001, 0x49646041, 0x20006081,  // +0x0024
    0x48634770, 0x22806801, 0x6002430A, 0x4A616801,  // +0x0034
    0x6002400A, 0x47702000, 0x2014B570, 0x680A495E,  // +0x0044
    0x600A4302, 0x2320684A, 0x604B4313, 0x2340684A,  // +0x0054
    0x604B4313, 0x4B594A53, 0xE0002401, 0x680D6013,  // +0x0064
    0xD1FB4025, 0x2620684D, 0x604D43B5, 0x2610684D,  // +0x0074
    0x604E432E, 0x4E534D52, 0xE0008035, 0x680D6013,  // +0x0084
    0xD1FB4025, 0x2310684A, 0x604A439A, 0x4002680A,  // +0x0094
    0xE099D000, 0xBD702000, 0x49472014, 0x4302680A,  // +0x00A4
    0x684A600A, 0x43132320, 0x684A604B, 0x43132340,  // +0x00B4
    0x4B3C604B, 0x4A41E001, 0x680A601A, 0xD4FA07D2,  // +0x00C4
    0x2320684A, 0x604A439A, 0x4002680A, 0x680AD004,  // +0x00D4
    0x60084310, 0x47702001, 0x47702000, 0x1C49B570,  // +0x00E4
    0x43992301, 0x681C4B34, 0x43252504, 0x2414601D,  // +0x00F4
    0x1C80E002, 0x1E891C92, 0xD0182900, 0x2610685D,  // +0x0104
    0x605E432E, 0x80058815, 0xE0014E26, 0x60354D2B,  // +0x0114
    0x07ED681D, 0x685DD4FA, 0x43B52610, 0x681D605D,  // +0x0124
    0xD0E64025, 0x43046818, 0x2001601C, 0x2000BD70,  // +0x0134
    0x1840BD70, 0xB5704770, 0x491F2014, 0x4302680A,  // +0x0144
    0x684A600A, 0x43132320, 0x684A604B, 0x43132340,  // +0x0154
    0x4A14604B, 0x24014B19, 0x6013E000, 0x4025680D,  // +0x0164
    0x684DD1FB, 0x43B52620, 0x684D604D, 0x432E2610,  // +0x0174
    0x2500604E, 0x80354E13, 0x6013E000, 0x4025680D,  // +0x0184
    0x684AD1FB, 0x439A2310, 0x680A604A, 0xD0004002,  // +0x0194
    0x2000E01A, 0x0000BD70, 0x40022000, 0x45670123,  // +0x01A4
    0xCDEF89AB, 0x40003000, 0x00005555, 0x00000FFF,  // +0x01B4
    0x40022010, 0xFFFFFEFF, 0x4002200C, 0x0000AAAA,  // +0x01C4
    0x000055AA, 0x1FFFF800, 0x4310680A, 0x20016008,  // +0x01D4
    0x0000BD70, 0x00000000,                          // +0x01E4
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
    0x200000AD,   // EraseSector
    0x200000F1,   // ProgramPage
    0x2000014B,   // SetRDP
    0x20000147,   // Verify
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