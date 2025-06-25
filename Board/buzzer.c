#include "buzzer.h"

// BUZZER IO初始化
void Buzzer_Init(void) {
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);   // 使能PC端口时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    // 使能PC端口时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    //

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;   // LED0端口配置
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // IO口速度为2MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);              // 根据设定参数初始化GPIO

    TIM_TimeBaseStructure.TIM_Period        = 400 - 1;              // 设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler     = 72 - 1;               // 设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;                    // 设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;   // TIM向上计数模式
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);                 // 根据Struct中指定的参数初始化TIMx的时间基数单位

    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;                   // 选择定时器模式:TIM脉冲宽度调制模式2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;            // 比较输出使能
    TIM_OCInitStructure.TIM_Pulse       = (uint16_t) ((TIM3->ARR) * 0.8f);   // 设置待装入捕获比较寄存器的脉冲值
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;               // 输出极性:TIM输出比较极性高
    TIM_OC4Init(TIM3, &TIM_OCInitStructure);                                 // 根据TIM_OCInitStruct中指定的参数初始化外设TIMx

    TIM_CtrlPWMOutputs(TIM3, ENABLE);                   // MOE 主输出使能
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);   // CH3预装载使能
    TIM_ARRPreloadConfig(TIM3, ENABLE);                 // 使能TIMx在ARR上的预装载寄存器
    TIM_Cmd(TIM3, ENABLE);                              // 使能TIM4
}
