#ifndef __TASK_KEY_H__
#define __TASK_KEY_H__

#include "Key.h"
#include "stm32f10x.h"

#define Key_SHORT_TIME  3     // 按键短按时间
#define Key_LONG_TIME   200   // 按键长按时间
#define Key_ALWAYS_TIME 20    // 按键一直按时间

void           Key_Task(void);                       // 按键处理任务
static uint8_t Key_ClickEvent(uint32_t key);         // 按键单击事件
static uint8_t Key_LongPressEvent(uint32_t key);     // 按键长按事件
static uint8_t Key_AlwaysPressEvent(uint32_t key);   // 按键一直按事件

#endif