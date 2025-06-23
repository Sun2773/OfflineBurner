#include "led.h"

static uint8_t LED_Mode = 0;   // LEDģʽ��־ 0 ����ģʽ

/**
 * @brief  LED IO��ʼ��
 * @note
 * @retval None
 */
void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(LED_RCCCLOCK, ENABLE);   // ʹ��PC�˿�ʱ��

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;   // �������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;    // IO���ٶ�Ϊ2MHz

    GPIO_InitStructure.GPIO_Pin = RUN_LED_PIN;
    GPIO_Init(RUN_LED_PORT, &GPIO_InitStructure);   // ��ʼ��
    LED_Off(RUN);

    GPIO_InitStructure.GPIO_Pin = ERR_LED_PIN;
    GPIO_Init(ERR_LED_PORT, &GPIO_InitStructure);   // ��ʼ��
    LED_Off(ERR);
}

/**
 * @brief  LED��������
 * @note   50msʱ��Ƭ
 * @retval None
 */
void LED_Task(void) {
    static uint16_t led_timer = 0;   // LED��ʱ��
    switch (LED_Mode) {
        case 0: {   // ����ģʽ
            /* ÿ100ms�л�һ�ε�״̬ */
            if (led_timer++ >= 20) {
                led_timer = 0;
                LED_OnOff(RUN);
            }
        } break;
    }
}
