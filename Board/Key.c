#include "Key.h"
#include "Tool.h"

/* 按键开关IO定义 */
static struct {
    GPIO_TypeDef* GPIOx;
    uint16_t      GPIO_Pin;
} Key_Gpio[] = {
    {KEY_1_PORT, KEY_1_PIN},
};

/**
 * @brief  按键初始化
 * @note
 * @retval None
 */
void Key_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(KEY_RCCCLOCK, ENABLE);   // 使能GPIO时钟

    /* GPIO初始化设置 */
    for (uint8_t i = 0; i < ArraySize(Key_Gpio); i++) {
        GPIO_InitStructure.GPIO_Pin  = Key_Gpio[i].GPIO_Pin;   // 引脚配置
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;          // 上拉输入
        GPIO_Init(Key_Gpio[i].GPIOx, &GPIO_InitStructure);     // 初始化引脚
    }

    return;
}

/**
 * @brief  获取按键值
 * @note
 * @retval 键值
 */
uint32_t Key_Get(void) {
    uint32_t key = 0;
    for (uint8_t i = 0; i < ArraySize(Key_Gpio); i++) {
        uint8_t val = !(Key_Gpio[i].GPIOx->IDR & Key_Gpio[i].GPIO_Pin);
        key |= val << i;
    }
    return key;
}
