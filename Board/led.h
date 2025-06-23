#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"

#define LED_RCCCLOCK RCC_APB2Periph_GPIOA

#define RUN_LED_PORT GPIOA
#define RUN_LED_PIN  GPIO_Pin_1

#define ERR_LED_PORT GPIOA
#define ERR_LED_PIN  GPIO_Pin_2

#define LED_On(led)    GPIO_ResetBits(led##_LED_PORT, led##_LED_PIN)           // 开灯
#define LED_Off(led)   GPIO_SetBits(led##_LED_PORT, led##_LED_PIN)             // 关灯
#define LED_OnOff(led) (led##_LED_PORT->ODR ^= led##_LED_PIN)                  // 转换灯
#define LED_Get(led)   !GPIO_ReadInputDataBit(led##_LED_PORT, led##_LED_PIN)   // 获取灯状态

void LED_Init(void);   // LED IO初始化
void LED_Task(void);   // LED运行任务

#endif
