#ifndef __TASK_BURNER_H__
#define __TASK_BURNER_H__

#include "stm32f10x.h"

#define BURNER_TARGET_ADDRESS 0x08000000   // 烧录目标地址

typedef struct {
    uint8_t  Online;       // 在线状态
    uint8_t  Start;        // 启动状态
    uint32_t ChipIdcode;   // 芯片ID
    uint8_t* Buffer;
} BurnerCtrl_t;

extern BurnerCtrl_t BurnerCtrl;

void Burner_Task(void);

#endif // __TASK_BURNER_H__