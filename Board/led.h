#ifndef __LED_H__
#define __LED_H__

#include "Tool.h"
#include "stm32f10x.h"

#define LED_RCCCLOCK RCC_APB2Periph_GPIOA

#define RUN_LED_PORT GPIOA
#define RUN_LED_PIN  GPIO_Pin_1

#define ERR_LED_PORT GPIOA
#define ERR_LED_PIN  GPIO_Pin_2

#define LED_RUN_On()     (TIM2->CCR2 = (1000))                         // 开灯
#define LED_RUN_Off()    (TIM2->CCR2 = (0))                            // 关灯
#define LED_RUN_OnOff()  (TIM2->CCR2 ? LED_RUN_Off() : LED_RUN_On())   // 转换灯
#define LED_RUN_Get()    (TIM2->CCR2 != (0))                           // 获取灯状态
#define LED_RUN_LUM(lum) (TIM2->CCR2 = (lum))                          // 设置灯亮度

#define LED_ERR_On()     (TIM2->CCR3 = (1000))                         // 开灯
#define LED_ERR_Off()    (TIM2->CCR3 = (0))                            // 关灯
#define LED_ERR_OnOff()  (TIM2->CCR3 ? LED_ERR_Off() : LED_ERR_On())   // 转换灯
#define LED_ERR_Get()    (TIM2->CCR3 != (0))                           // 获取灯状态
#define LED_ERR_LUM(lum) (TIM2->CCR3 = (lum))                          // 设置灯亮度

#define LED_On(led)       LED_##led##_On()       // 开灯
#define LED_Off(led)      LED_##led##_Off()      // 关灯
#define LED_OnOff(led)    LED_##led##_OnOff()    // 转换灯
#define LED_Get(led)      LED_##led##_Get()      // 获取灯状态
#define LED_Lum(led, lum) LED_##led##_LUM(lum)   // 设置灯亮度

void LED_Init(void);   // LED IO初始化

#endif
