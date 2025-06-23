#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"

#define LED_RCCCLOCK RCC_APB2Periph_GPIOA

#define RUN_LED_PORT GPIOA
#define RUN_LED_PIN  GPIO_Pin_1

#define ERR_LED_PORT GPIOA
#define ERR_LED_PIN  GPIO_Pin_2

#define LED_On(led)    GPIO_ResetBits(led##_LED_PORT, led##_LED_PIN)           // ����
#define LED_Off(led)   GPIO_SetBits(led##_LED_PORT, led##_LED_PIN)             // �ص�
#define LED_OnOff(led) (led##_LED_PORT->ODR ^= led##_LED_PIN)                  // ת����
#define LED_Get(led)   !GPIO_ReadInputDataBit(led##_LED_PORT, led##_LED_PIN)   // ��ȡ��״̬

void LED_Init(void);   // LED IO��ʼ��
void LED_Task(void);   // LED��������

#endif
