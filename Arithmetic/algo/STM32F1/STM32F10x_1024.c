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
//   Init         @ +0x0005 (size: 68 bytes)
//   UnInit       @ +0x0049 (size: 28 bytes)
//   EraseChip    @ +0x0065 (size: 110 bytes)
//   EraseSector  @ +0x00D3 (size: 134 bytes)
//   ProgramPage  @ +0x0159 (size: 182 bytes)

static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4603B510, 0x04C00CD8, 0x444C4C80, 0x20006020,  // +0x0004
    0x60204C7F, 0x6060487F, 0x6060487F, 0x6460487D,  // +0x0014
    0x6460487D, 0x69C04620, 0x0F04F010, 0xF245D108,  // +0x0024
    0x4C7A5055, 0x20066020, 0xF6406060, 0x60A070FF,  // +0x0034
    0xBD102000, 0x48724601, 0xF0406900, 0x4A700080,  // +0x0044
    0x46106110, 0xF0406D00, 0x65100080, 0x47702000,  // +0x0054
    0x6900486B, 0x0004F040, 0x61084969, 0x69004608,  // +0x0064
    0x0040F040, 0xE0036108, 0x20AAF64A, 0x60084967,  // +0x0074
    0x68C04863, 0x0F01F010, 0x4861D1F6, 0xF0206900,  // +0x0084
    0x495F0004, 0x46086108, 0xF0406D00, 0x65080004,  // +0x0094
    0x6D004608, 0x0040F040, 0xE0036508, 0x20AAF64A,  // +0x00A4
    0x6008495A, 0x6CC04856, 0x0F01F010, 0x4854D1F6,  // +0x00B4
    0xF0206D00, 0x49520004, 0x20006508, 0x46014770,  // +0x00C4
    0x4448484E, 0xF5006800, 0x42812000, 0x484CD21C,  // +0x00D4
    0xF0406900, 0x4A4A0002, 0x46106110, 0x69006141,  // +0x00E4
    0x0040F040, 0xE0036110, 0x20AAF64A, 0x60104A47,  // +0x00F4
    0x68C04843, 0x0F01F010, 0x4841D1F6, 0xF0206900,  // +0x0104
    0x4A3F0002, 0xE01B6110, 0x6D00483D, 0x0002F040,  // +0x0114
    0x65104A3B, 0x65414610, 0xF0406D00, 0x65100040,  // +0x0124
    0xF64AE003, 0x4A3920AA, 0x48356010, 0xF0106CC0,  // +0x0134
    0xD1F60F01, 0x6D004832, 0x0002F020, 0x65104A30,  // +0x0144
    0x47702000, 0x4603B510, 0xF0201C48, 0x482B0101,  // +0x0154
    0x68004448, 0x2000F500, 0xD2264283, 0x4828E022,  // +0x0164
    0xF0406900, 0x4C260001, 0x88106120, 0xBF008018,  // +0x0174
    0x68C04823, 0x0F01F010, 0x4821D1FA, 0xF0206900,  // +0x0184
    0x4C1F0001, 0x46206120, 0xF01068C0, 0xD0060F14,  // +0x0194
    0x68C04620, 0x0014F040, 0x200160E0, 0x1C9BBD10,  // +0x01A4
    0x1E891C92, 0xD1DA2900, 0xE022E025, 0x6D004814,  // +0x01B4
    0x0001F040, 0x65204C12, 0x80188810, 0x4810BF00,  // +0x01C4
    0xF0106CC0, 0xD1FA0F01, 0x6D00480D, 0x0001F020,  // +0x01D4
    0x65204C0B, 0x6CC04620, 0x0F14F010, 0x4620D006,  // +0x01E4
    0xF0406CC0, 0x64E00014, 0xE7D72001, 0x1C921C9B,  // +0x01F4
    0x29001E89, 0x2000D1DA, 0x0000E7D0, 0x00000004,  // +0x0204
    0x40022000, 0x45670123, 0xCDEF89AB, 0x40003000,  // +0x0214
    0x00000000                                       // +0x0224
    // clang-format on
};

// Flash sector information
static const sector_info_t sector_info[] = {
    {0x0800, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_1024_ = {
    0x20000005,   // Init
    0x20000049,   // UnInit
    0x20000065,   // EraseChip
    0x200000D3,   // EraseSector
    0x20000159,   // ProgramPage
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