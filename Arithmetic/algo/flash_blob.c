#include "flash_blob.h"
#include "stdlib.h"

extern const program_target_t _stm32f10x_opt_;
extern const program_target_t _stm32f10x_flash_;

const FlashBlobList_t FlashBlobList[] = {
    {
        /* STM32F10x_LD: 小容量产品 16K~32K */
        .DevId         = 0x412,                // 设备ID
        .Name          = "STM32F10x_LD",       // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,           // Flash大小寄存器地址
        .prog_flash    = &_stm32f10x_flash_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,     // 选项字编程算法
    },
    {
        /* STM32F10x_MD: 中容量产品 64K~128K */
        .DevId         = 0x410,                // 设备ID
        .Name          = "STM32F10x_MD",       // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,           // Flash大小寄存器地址
        .prog_flash    = &_stm32f10x_flash_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,     // 选项字编程算法
    },

    {
        /* STM32F10x_HD: 高容量产品 256K~512K */
        .DevId         = 0x414,                // 产品ID
        .Name          = "STM32F10x_HD",       // 产品名称
        .FlashSizeAddr = 0x1FFFF7E0,           // Flash大小寄存器地址
        .prog_flash    = &_stm32f10x_flash_,   // Flash编程算法
        .prog_opt      = &_stm32f10x_opt_,     // 选项字编程算法
    },
    // {
    //     /* STM32F10x_XL: 互联型产品 */
    //     .DevId         = 0x418,                // 产品ID
    //     .Name          = "STM32F10x_XL",       // 产品名称
    //     .FlashSizeAddr = 0x1FFFF7E0,           // Flash大小寄存器地址
    //     .prog_flash    = &_stm32f10x_flash_,   // Flash编程算法
    //     .prog_opt      = &_stm32f10x_opt_,     // 选项字编程算法
    // },
    {
        /* STM32F405xx/07xx STM32F415xx/17xx */
        .DevId         = 0x413,                                 // 产品ID
        .Name          = "STM32F405xx/07xx STM32F415xx/17xx",   // 产品名称
        .FlashSizeAddr = 0x1FFF7A22,                            // Flash大小寄存器地址
        .prog_flash    = NULL,                                  // Flash编程算法
        .prog_opt      = NULL,                                  // 选项字编程算法
    },
};

/**
 * @brief  根据设备ID获取对应的Flash编程算法
 * @note
 * @param  id: 设备ID
 * @retval 指向FlashBlobList_t的指针，如果未找到则返回NULL
 */
FlashBlobList_t* FlashBlob_GetForId(uint16_t id) {
    for (uint8_t i = 0; i < sizeof(FlashBlobList) / sizeof(FlashBlobList[0]); i++) {
        if (FlashBlobList[i].DevId == id) {
            return (FlashBlobList_t*) &FlashBlobList[i];
        }
    }
    return NULL;   // 未找到对应的设备ID
}
