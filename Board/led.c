#include "led.h"

/**
 * @brief  LED IO初始化
 * @note
 * @retval None
 */
void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(LED_RCCCLOCK, ENABLE);   // 使能PC端口时钟

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;    // IO口速度为2MHz

    GPIO_InitStructure.GPIO_Pin = RUN_LED_PIN;
    GPIO_Init(RUN_LED_PORT, &GPIO_InitStructure);   // 初始化
    LED_Off(RUN);

    GPIO_InitStructure.GPIO_Pin = ERR_LED_PIN;
    GPIO_Init(ERR_LED_PORT, &GPIO_InitStructure);   // 初始化
    LED_Off(ERR);
}

/**
 * @brief  LED运行任务
 * @note   500ms时间片
 * @retval None
 */
void LED_Task(void) {
    LED_OnOff(RUN);
}
