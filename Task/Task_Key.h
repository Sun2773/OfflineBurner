#ifndef __TASK_KEY_H__
#define __TASK_KEY_H__

#include "Key.h"
#include "stm32f10x.h"

#define Key_SHORT_TIME  5     // �����̰�ʱ��
#define Key_LONG_TIME   100   // ��������ʱ��
#define Key_ALWAYS_TIME 20    // ����һֱ��ʱ��

void           Key_Task(void);                       // ������������
static uint8_t Key_ClickEvent(uint32_t key);         // ���������¼�
static uint8_t Key_LongPressEvent(uint32_t key);     // ���������¼�
static uint8_t Key_AlwaysPressEvent(uint32_t key);   // ����һֱ���¼�

#endif