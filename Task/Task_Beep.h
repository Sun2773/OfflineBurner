#ifndef __BEEP_H__
#define __BEEP_H__

#include "stm32f4xx.h"

typedef enum {
    BEEP_Stop,                  // 停止
    BEEP_VeryShort,             // 超短鸣一次    50ms
    BEEP_Short,                 // 短鸣一次      100ms
    BEEP_Medium,                // 中鸣一次      500ms
    BEEP_Long,                  // 长鸣一次      1s
    BEEP_DoubleShort,           // 双短鸣一次
    BEEP_DoubleShortContinue,   // 双短鸣持续
    BEEP_Alarm,                 // 报警音持续
    BEEP_WarningTone_1,
    BEEP_WarningTone_2,
    BEEP_WarningTone_3,
    BEEP_WarningTone_4,
} BEEP_MODE;

void Beep_Task(void);
void Beep(BEEP_MODE mode);

#endif
