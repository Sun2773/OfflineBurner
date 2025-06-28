#ifndef __TASK_BURNER_H__
#define __TASK_BURNER_H__

#include "flash_blob.h"
#include "stdlib.h"
#include "stm32f10x.h"

#define BURNER_TARGET_ADDRESS  0x08000000     // 烧录目标地址
#define BURNER_AUTO_START_TIME (1000 / 100)   // 烧录目标地址
#define BURNER_AUTO_END_TIME   (500 / 100)    // 烧录目标地址

#define BURNER_STATE_IDLE    0   // 空闲状态
#define BURNER_STATE_START   1   // 启动烧录
#define BURNER_STATE_RUNNING 2   // 烧录中状态
#define BURNER_STATE_FINISH  3   // 烧录完成状态
#define BURNER_STATE_READY   4   // 准备烧录状态

typedef struct {
    uint8_t  Online;       // 在线状态
    uint8_t  State;        // 工作状态
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
    uint16_t         FlashSize;    // Flash大小(Kb)
    uint8_t*         Buffer;       // 烧录数据缓冲区
    uint16_t         Error;        // 错误码
    int16_t          StartTimer;   // 启动计时器
    int16_t          EndTimer;     // 结束计时器
    FlashBlobList_t* FlashBlob;    // 当前Flash编程算法
} BurnerCtrl_t;

extern BurnerCtrl_t BurnerCtrl;

void Burner_Task(void);
void Burner_Detection(void);
void Burner_Exe(void);

#endif   // __TASK_BURNER_H__