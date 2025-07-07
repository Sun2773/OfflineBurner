#include "led.h"

/**
 * @brief  LED IO初始化
 * @note
 * @retval None
 */
void LED_Init(void) {
    // GPIO_InitTypeDef GPIO_InitStructure;
    // RCC_APB2PeriphClockCmd(LED_RCCCLOCK, ENABLE);   // 使能PC端口时钟

    // GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;   // 推挽输出
    // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;    // IO口速度为2MHz

    // GPIO_InitStructure.GPIO_Pin = RUN_LED_PIN;
    // GPIO_Init(RUN_LED_PORT, &GPIO_InitStructure);   // 初始化
    // LED_Off(RUN);

    // GPIO_InitStructure.GPIO_Pin = ERR_LED_PIN;
    // GPIO_Init(ERR_LED_PORT, &GPIO_InitStructure);   // 初始化
    // LED_Off(ERR);

    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // 使能PC端口时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    //

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2;   // LED0端口配置
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // IO口速度为2MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);              // 根据设定参数初始化GPIO

    TIM_TimeBaseStructure.TIM_Period        = 1000 - 1;             // 设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler     = 72 - 1;               // 设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;                    // 设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;   // TIM向上计数模式
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);                 // 根据Struct中指定的参数初始化TIMx的时间基数单位

    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM2;          // 选择定时器模式:TIM脉冲宽度调制模式2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   // 比较输出使能
    TIM_OCInitStructure.TIM_Pulse       = 0;                        // 设置待装入捕获比较寄存器的脉冲值
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;      // 输出极性:TIM输出比较极性高
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);                        // 根据TIM_OCInitStruct中指定的参数初始化外设TIMx
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);                        // 根据TIM_OCInitStruct中指定的参数初始化外设TIMx

    TIM_CtrlPWMOutputs(TIM2, ENABLE);                   // MOE 主输出使能
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);   // CH3预装载使能
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);   // CH3预装载使能
    TIM_ARRPreloadConfig(TIM2, ENABLE);                 // 使能TIMx在ARR上的预装载寄存器
    TIM_Cmd(TIM2, ENABLE);                              // 使能TIM4
}
