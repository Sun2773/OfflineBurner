#include "Key.h"
#include "Tool.h"

/* ��������IO���� */
static struct {
    GPIO_TypeDef* GPIOx;
    uint16_t      GPIO_Pin;
} Key_Gpio[] = {
    {KEY_1_PORT, KEY_1_PIN},
};

/**
 * @brief  ������ʼ��
 * @note
 * @retval None
 */
void Key_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(KEY_RCCCLOCK, ENABLE);   // ʹ��GPIOʱ��

    /* GPIO��ʼ������ */
    for (uint8_t i = 0; i < ArraySize(Key_Gpio); i++) {
        GPIO_InitStructure.GPIO_Pin  = Key_Gpio[i].GPIO_Pin;   // ��������
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;          // ��������
        GPIO_Init(Key_Gpio[i].GPIOx, &GPIO_InitStructure);     // ��ʼ������
    }

    return;
}

/**
 * @brief  ��ȡ����ֵ
 * @note
 * @retval ��ֵ
 */
uint32_t Key_Get(void) {
    uint32_t key = 0;
    for (uint8_t i = 0; i < 1; i++) {
        uint8_t val = !(Key_Gpio[i].GPIOx->IDR & Key_Gpio[i].GPIO_Pin);
        key |= val << i;
    }
    return key;
}
