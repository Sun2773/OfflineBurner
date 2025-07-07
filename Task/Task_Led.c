#include "Task_Led.h"
#include "hw_config.h"
#include "led.h"
#include "math.h"

float breathing_sine(float t, float T) {
    float pi = 3.1415926535f;
    return (1.0f + sinf(2.0f * pi * t / T - pi / 2)) / 2.0f;
}

float gamma_correct(float brightness, float gamma) {
    return powf(brightness, gamma);   // gamma通常取2.2
}

/**
 * @brief  LED运行任务
 * @note   50ms时间片
 * @retval None
 */
void LED_Task(void) {
    static uint16_t tim = 0;
    if (USB_StateGet() == 0) {
        if (tim++ >= 10) {   //
            LED_OnOff(RUN);
            tim = 0;
        }
    } else {
        LED_Lum(RUN, (uint16_t) (1000 * gamma_correct(breathing_sine(tim, 80), 2.2)));   // 设置LED亮度
        if (++tim > 80) {                                                                // 亮度变化周期
            tim = 0;                                                                     // 重置计时器
        }
    }
}