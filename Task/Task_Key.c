#include "Task_Key.h"

/**
 * @brief  ������������
 * @note   10msʱ��Ƭ
 * @retval None
 */
void Key_Task(void) {
    static uint32_t key_last    = 0;   // ������һ�ε�״̬
    static uint8_t  key_trigger = 0;   // ��������
    static uint16_t key_tim     = 0;   // ������ʱ��

    uint32_t key = Key_Get();

    if ((key & KEY_MASK) != 0) {
        if (key_trigger == 0) {
            if (++key_tim >= Key_LONG_TIME) {
                key_trigger = 1;
                key_tim     = 0;
                if (Key_LongPressEvent(key_last)) {
                    // Beep(BEEP_Short);
                }
            }
        } else {
            if (++key_tim >= Key_LONG_TIME + Key_ALWAYS_TIME) {
                key_tim = Key_LONG_TIME;
                if (Key_AlwaysPressEvent(key_last)) {
                    // Beep(BEEP_Short);
                }
            }
        }
    } else {
        if (key_trigger == 0 && key_tim > Key_SHORT_TIME) {
            if (Key_ClickEvent(key_last)) {
                // Beep(BEEP_Short);
            }
        }
        key_trigger = 0;
        key_tim     = 0;
    }

    key_last = (key & KEY_MASK);
}

/**
 * @brief  ���������¼�
 * @note
 * @param  key: ������ֵ
 * @retval 1: �¼�����ɹ�, 0: �¼�����ʧ��
 */
static uint8_t Key_ClickEvent(uint32_t key) {
    return 1;
}

/**
 * @brief  ���������¼�
 * @note
 * @param  key: ������ֵ
 * @retval 1: �¼�����ɹ�, 0: �¼�����ʧ��
 */
static uint8_t Key_LongPressEvent(uint32_t key) {
    return 1;
}

/**
 * @brief  ����һֱ���¼�
 * @note
 * @param  key: ������ֵ
 * @retval 1: �¼�����ɹ�, 0: �¼�����ʧ��
 */
static uint8_t Key_AlwaysPressEvent(uint32_t key) {
    return 0;
}
