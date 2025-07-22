#include "flash_blob.h"
#include "stdlib.h"
#include "string.h"

extern const program_target_t _stm32f0xx_opt_;
extern const program_target_t _stm32f0xx_64_;
extern const program_target_t _stm32f0xx_256_2k_;

extern const program_target_t _stm32f10x_opt_;
extern const program_target_t _stm32f10x_128_;
extern const program_target_t _stm32f10x_512_;

extern const program_target_t _stm32f2xx_opt_;
extern const program_target_t _stm32f2xx_1024_;

extern const program_target_t _stm32f3xx_opt_;
extern const program_target_t _stm32f3xx_512_;

extern const program_target_t _stm32f40xxx_41xxx_opt_;
extern const program_target_t _stm32f4xx_512_;
extern const program_target_t _stm32f4xx_1024_;

const FlashBlobList_t FlashBlobList[] = {
    {
        /* STM32F03x */
        .DevId         = 0x444,              // 设备ID
        .Name          = "STM32F03x",        // 产品名称
        .FlashSizeAddr = 0x1FFFF7CC,         // Flash大小寄存器地址
        .FlashSize     = {16, 64},           // Flash大小范围
        .prog_flash    = &_stm32f0xx_64_,    // Flash编程算法
        .prog_opt      = &_stm32f0xx_opt_,   // 选项字编程算法
    },
    {
        /* STM32F04x */
        .DevId         = 0x445,              // 设备ID
        .Name          = "STM32F04x",        // 产品名称
        .FlashSizeAddr = 0x1FFFF7CC,         // Flash大小寄存器地址
        .FlashSize     = {16, 64},           // Flash大小范围
        .prog_flash    = &_stm32f0xx_64_,    // Flash编程算法
        .prog_opt      = &_stm32f0xx_opt_,   // 选项字编程算法
    },
    {
        /* STM32F05x */
        .DevId         = 0x440,              // 设备ID
        .Name          = "STM32F05x",        // 产品名称
        .FlashSizeAddr = 0x1FFFF7CC,         // Flash大小寄存器地址
        .FlashSize     = {16, 64},           // Flash大小范围
        .prog_flash    = &_stm32f0xx_64_,    // Flash编程算法
        .prog_opt      = &_stm32f0xx_opt_,   // 选项字编程算法
    },
    {
        /* STM32F07x */
        .DevId         = 0x448,                 // 设备ID
        .Name          = "STM32F07x",           // 产品名称
        .FlashSizeAddr = 0x1FFFF7CC,            // Flash大小寄存器地址
        .FlashSize     = {64, 256},             // Flash大小范围
        .prog_flash    = &_stm32f0xx_256_2k_,   // Flash编程算法
        .prog_opt      = &_stm32f0xx_opt_,      // 选项字编程算法
    },
    {
        /* STM32F09x */
        .DevId         = 0x442,                 // 设备ID
        .Name          = "STM32F09x",           // 产品名称
        .FlashSizeAddr = 0x1FFFF7CC,            // Flash大小寄存器地址
        .FlashSize     = {64, 256},             // Flash大小范围
        .prog_flash    = &_stm32f0xx_256_2k_,   // Flash编程算法
        .prog_opt      = &_stm32f0xx_opt_,      // 选项字编程算法
    },
    {
        /* STM32F10x_LD: 小容量产品 16K~32K */
        .DevId         = 0x412,              // 设备ID
        .Name          = "STM32F10x_LD",     // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,         // Flash大小寄存器地址
        .FlashSize     = {16, 32},           // Flash大小范围
        .prog_flash    = &_stm32f10x_128_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,   // 选项字编程算法
    },
    {
        /* STM32F10x_MD: 中容量产品 64K~128K */
        .DevId         = 0x410,              // 设备ID
        .Name          = "STM32F10x_MD",     // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,         // Flash大小寄存器地址
        .FlashSize     = {64, 128},          // Flash大小范围
        .prog_flash    = &_stm32f10x_128_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,   // 选项字编程算法
    },
    {
        /* STM32F10x_HD: 高容量产品 256K~512K */
        .DevId         = 0x414,              // 产品ID
        .Name          = "STM32F10x_HD",     // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,         // Flash大小寄存器地址
        .FlashSize     = {256, 512},         // Flash大小范围
        .prog_flash    = &_stm32f10x_512_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,   // 选项字编程算法
    },
    {
        /* STM32F10x_XL: 互联型产品 */
        .DevId         = 0x418,              // 产品ID
        .Name          = "STM32F10x_XL",     // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,         // Flash大小寄存器地址
        .FlashSize     = {128, 256},         // Flash大小范围
        .prog_flash    = &_stm32f10x_512_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,   // 选项字编程算法
    },
    {
        /* STM32F2xx */
        .DevId         = 0x411,               // 产品ID
        .Name          = "STM32F2xx",         // 产品名称
        .FlashSizeAddr = 0x1FFF7A22,          // Flash大小寄存器地址
        .FlashSize     = {128, 1024},         // Flash大小范围
        .prog_flash    = &_stm32f2xx_1024_,   // Flash编程算法
        .prog_opt      = &_stm32f2xx_opt_,    // 选项字编程算法
    },
    {
        /* STM32F3xx */
        .DevId         = 0x432,              // 产品ID
        .Name          = "STM32F3xx",        // 产品名称
        .FlashSizeAddr = 0x1FFFF7CC,         // Flash大小寄存器地址
        .FlashSize     = {16, 512},          // Flash大小范围
        .prog_flash    = &_stm32f3xx_512_,   // Flash编程算法
        .prog_opt      = &_stm32f3xx_opt_,   // 选项字编程算法
    },
    {
        /* STM32F405xx/07xx STM32F415xx/17xx */
        .DevId         = 0x413,                                 // 产品ID
        .Name          = "STM32F405xx/07xx STM32F415xx/17xx",   // 产品名称
        .FlashSizeAddr = 0x1FFF7A22,                            // Flash大小寄存器地址
        .FlashSize     = {128, 512},                            // Flash大小范围
        .prog_flash    = &_stm32f4xx_512_,                      // Flash编程算法
        .prog_opt      = &_stm32f40xxx_41xxx_opt_,              // 选项字编程算法
    },
    {
        /* STM32F405xx/07xx STM32F415xx/17xx */
        .DevId         = 0x413,                                 // 产品ID
        .Name          = "STM32F405xx/07xx STM32F415xx/17xx",   // 产品名称
        .FlashSizeAddr = 0x1FFF7A22,                            // Flash大小寄存器地址
        .FlashSize     = {1024, 1024},                          // Flash大小范围
        .prog_flash    = &_stm32f4xx_1024_,                     // Flash编程算法
        .prog_opt      = &_stm32f40xxx_41xxx_opt_,              // 选项字编程算法
    },
};

/**
 * @brief  根据设备ID和Flash大小获取对应的Flash编程算法
 * @note
 * @param  id: 设备ID
 * @param  flash_size: Flash大小
 * @retval 指向FlashBlobList_t的指针，如果未找到则返回NULL
 */
FlashBlobList_t* FlashBlob_Get(uint16_t id, uint16_t flash_size) {
    for (size_t i = 0; i < sizeof(FlashBlobList) / sizeof(FlashBlobList[0]); i++) {
        if ((FlashBlobList[i].DevId == id) &&
            ((flash_size == 0) ||
             ((flash_size >= FlashBlobList[i].FlashSize[0]) &&
              (flash_size <= FlashBlobList[i].FlashSize[1])))) {
            return (FlashBlobList_t*) &FlashBlobList[i];
        }
    }
    return NULL;   // 未找到对应的编程算法
}

/**
 * @brief  获取算法列表字符串
 * @note
 * @param  str: 字符串缓冲区
 * @retval None
 */
void FlashBlob_ListStr(char* str) {
    *str = '\0';   // 清空字符串
    for (size_t i = 0; i < sizeof(FlashBlobList) / sizeof(FlashBlobList[0]); i++) {
        if (strstr(str, FlashBlobList[i].Name) != NULL) {
            /* 如果设备名称已存在，则跳过 */
            continue;
        }
        strcat(str, FlashBlobList[i].Name);   // 添加设备名称
        strcat(str, "\r\n");                  // 添加换行符
    }
}