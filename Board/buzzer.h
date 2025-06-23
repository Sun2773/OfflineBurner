#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "stm32f10x.h"

#define BUZZER_RCCCLOCK RCC_AHB1Periph_GPIOB
#define BUZZER_PORT     GPIOB
#define BUZZER_PIN      GPIO_Pin_1

#define BUZZER_CTRL (BIT_VAL(TIM3->CCER, 12))   // ����������

#define Buzzer_Freq(freq)  (TIM3->PSC = (uint16_t) (720000 / freq) - 1)
#define Buzzer_Volume(vol) (TIM3->CCR4 = vol - 1)
#define Beep_On()          (BUZZER_CTRL = 1)
#define Beep_Off()         (BUZZER_CTRL = 0)

typedef enum {
    BEEP_Stop,                  // ����һ��      100ms
    BEEP_UltraShort,            // ����һ��      100ms
    BEEP_Short,                 // ����һ��      100ms
    BEEP_Medium,                // ����һ��      500ms
    BEEP_Long,                  // ����һ��      1s
    BEEP_DoubleShort,           // ˫����һ��
    BEEP_DoubleShortContinue,   // ˫��������

} BEEP_MODE;

void Buzzer_Init(void);   // ��ʼ��

#endif