#ifndef __BEEP_H__
#define __BEEP_H__

#include "stm32f4xx.h"

typedef enum {
    BEEP_Stop,                  // ֹͣ
    BEEP_VeryShort,             // ������һ��    50ms
    BEEP_Short,                 // ����һ��      100ms
    BEEP_Medium,                // ����һ��      500ms
    BEEP_Long,                  // ����һ��      1s
    BEEP_DoubleShort,           // ˫����һ��
    BEEP_DoubleShortContinue,   // ˫��������
    BEEP_Alarm,                 // ����������
    BEEP_WarningTone_1,
    BEEP_WarningTone_2,
    BEEP_WarningTone_3,
    BEEP_WarningTone_4,
} BEEP_MODE;

void Beep_Task(void);
void Beep(BEEP_MODE mode);

#endif
