#ifndef __TASK_BURNER_H__
#define __TASK_BURNER_H__

#include "flash_blob.h"
#include "stdlib.h"
#include "stm32f10x.h"

#define BURNER_TARGET_ADDRESS 0x08000000   // 烧录目标地址

typedef struct {
    uint8_t  Online;       // 在线状态
    uint8_t  Start;        // 启动状态
    uint32_t ChipIdcode;   // 芯片ID
    uint32_t CPUID;        //
    union {
        uint32_t DBGMCU_IDCODE;   // 调试MCU IDCODE寄存器
        struct {
            uint16_t DEV_ID : 12;   // 设备ID
            uint16_t        : 4;    // 保留位
            uint16_t REV_ID : 16;   // 版本ID
        };
    };
    uint16_t FlashSize;   // Flash大小(Kb)
    uint8_t* Buffer;
    uint16_t Error;
    FlashBlobList_t* FlashBlob;   // 当前Flash编程算法
} BurnerCtrl_t;

extern BurnerCtrl_t BurnerCtrl;

void Burner_Task(void);

#endif   // __TASK_BURNER_H__