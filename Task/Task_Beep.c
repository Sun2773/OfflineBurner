#include "beep.h"

/*�������*/

const uint32_t _BEEP_Stop[]                = {0};                                     // ֹͣ
const uint32_t _BEEP_VeryShort[]           = {5, 0};                                  // ������һ��    5ms
const uint32_t _BEEP_Short[]               = {100, 0};                                // ����һ��      100ms
const uint32_t _BEEP_Medium[]              = {500, 0};                                // ����һ��      500ms
const uint32_t _BEEP_Long[]                = {1000, 0};                               // ����һ��      1s
const uint32_t _BEEP_DoubleShort[]         = {80, 160, 80, 680, 0};                   // ˫����һ��
const uint32_t _BEEP_DoubleShortContinue[] = {80, 160, 80, 680, 0xFFFFFFFF};          // ˫��������
const uint32_t _BEEP_Alarm[]               = {80, 50, 80, 50, 80, 600, 0xFFFFFFFF};   // ����������
const uint32_t _BEEP_WarningTone_1[]       = {500, 160, 80, 160, 80, 0};              // ��ʾ��1
const uint32_t _BEEP_WarningTone_2[]       = {80, 50, 80, 50, 80, 0};                 // ��ʾ��2
const uint32_t _BEEP_WarningTone_3[]       = {80, 50, 80, 0};                         // ��ʾ��3
const uint32_t _BEEP_WarningTone_4[]       = {150, 50, 80, 0};                        // ��ʾ��4

const uint32_t* BeepVoice[] = {

    _BEEP_Stop,
    _BEEP_VeryShort,
    _BEEP_Short,
    _BEEP_Medium,
    _BEEP_Long,
    _BEEP_DoubleShort,
    _BEEP_DoubleShortContinue,
    _BEEP_Alarm,
    _BEEP_WarningTone_1,
    _BEEP_WarningTone_2,
    _BEEP_WarningTone_3,
    _BEEP_WarningTone_4,
};

/*ϵͳ����ָʾ������*/
void Task_Beep(void) {
    BaseType_t xReturn    = pdTRUE;
    uint32_t   notify     = 0;
    uint32_t   delay      = portMAX_DELAY;
    uint32_t*  task1      = NULL;
    uint32_t*  task2      = NULL;
    uint8_t    task1Index = 0;
    uint8_t    task2Index = 0;

    while (1) {
        xReturn = xTaskNotifyWait(pdFALSE, 0xFFFFF, &notify, delay);

        if (xReturn == pdTRUE) {
            task1 = (uint32_t*) notify;
            if (task1[0] == 0) {
                task1 = 0;
                task2 = 0;
            }
            task1Index = 0;
        }

        if (task1) {
            if (task1[task1Index] == 0) {
                delay = 0;
                task1 = 0;
                Beep_Off();
            } else if (task1[task1Index] == 0xFFFFFFFF) {
                Beep_Off();
                delay      = 0;
                task2      = (uint32_t*) task1;
                task2Index = 0;
                task1      = 0;
            } else {
                if (task1Index % 2) {
                    Beep_Off();
                } else {
                    Beep_On();
                }
                delay = task1[task1Index++];
            }
        } else if (task2) {
            if (task2[task2Index] == 0) {
                delay = 0;
                task2 = 0;
                Beep_Off();
            } else if (task2[task2Index] == 0xFFFFFFFF) {
                Beep_Off();
                delay      = 0;
                task2Index = 0;
            } else {
                if (task2Index % 2) {
                    Beep_Off();
                } else {
                    Beep_On();
                }
                delay = task2[task2Index++];
            }
        } else {
            delay = portMAX_DELAY;
            Beep_Off();
        }
    }
}

void Beep(BEEP_MODE mode) {
    xTaskNotify(Task_Beep_Handle, (uint32_t) BeepVoice[mode], eSetValueWithOverwrite);
}

