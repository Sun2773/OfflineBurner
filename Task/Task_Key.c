#include "Task_Key.h"

/**
 * @brief  按键处理任务
 * @note   10ms时间片
 * @retval None
 */
void Key_Task(void) {
    static uint32_t key_last    = 0;   // 按键上一次的状态
    static uint8_t  key_trigger = 0;   // 按键触发
    static uint16_t key_tim     = 0;   // 按键计时器

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
 * @brief  按键单击事件
 * @note
 * @param  key: 触发键值
 * @retval 1: 事件处理成功, 0: 事件处理失败
 */
static uint8_t Key_ClickEvent(uint32_t key) {
    return 1;
}

/**
 * @brief  按键长按事件
 * @note
 * @param  key: 触发键值
 * @retval 1: 事件处理成功, 0: 事件处理失败
 */
static uint8_t Key_LongPressEvent(uint32_t key) {
    return 1;
}

/**
 * @brief  按键一直按事件
 * @note
 * @param  key: 触发键值
 * @retval 1: 事件处理成功, 0: 事件处理失败
 */
static uint8_t Key_AlwaysPressEvent(uint32_t key) {
    return 0;
}
