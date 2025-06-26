#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "stm32f10x.h"
#include "Tool.h"

#define BUZZER_RCCCLOCK RCC_AHB1Periph_GPIOB
#define BUZZER_PORT     GPIOB
#define BUZZER_PIN      GPIO_Pin_1

#define BUZZER_CTRL (BIT_VAL(TIM3->CCER, 12))   // 蜂鸣器开关

#define Buzzer_Freq(freq)  (TIM3->PSC = (uint16_t) (720000 / freq) - 1)
#define Buzzer_Volume(vol) (TIM3->CCR4 = vol - 1)
#define Beep_On()          (BUZZER_CTRL = 1)
#define Beep_Off()         (BUZZER_CTRL = 0)

typedef enum {
    BEEP_Stop,                  // 短鸣一次      100ms
    BEEP_UltraShort,            // 短鸣一次      100ms
    BEEP_Short,                 // 短鸣一次      100ms
    BEEP_Medium,                // 中鸣一次      500ms
    BEEP_Long,                  // 长鸣一次      1s
    BEEP_DoubleShort,           // 双短鸣一次
    BEEP_DoubleShortContinue,   // 双短鸣持续

} BEEP_MODE;

void Buzzer_Init(void);   // 初始化

#endif