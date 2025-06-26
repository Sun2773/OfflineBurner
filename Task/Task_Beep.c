#include "Task_Beep.h"
#include "buzzer.h"

static uint16_t Beep_Timer = 0;

/**
 * @brief  蜂鸣器任务
 * @note   10ms周期执行
 * @retval None
 */
void Beep_Task(void) {
    if (Beep_Timer > 0) {
        Beep_On();
        Beep_Timer--;
    } else {
        Beep_Off();
    }
}

void Beep(uint16_t delay) {
    Beep_Timer = delay;
}
