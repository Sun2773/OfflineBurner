/***
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* 文件名:     Key_Switch.h
* 描述:       按键开关驱动头文件
* 版本号:     v0.1
* 作者:       孙威
* 创建日期:   2021/4/18
*
* @青岛程宁新能源科技有限公司
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**/

#ifndef __KEY_H__
#define __KEY_H__

#include "stm32f10x.h"

/* 键值定义 */
#define KEY_MASK 0x01   // 按键掩码
#define KEY_1    0x01   // 中键

/* 按键时钟 */
#define KEY_RCCCLOCK RCC_APB2Periph_GPIOA
/* 按键1引脚 */
#define KEY_1_PORT GPIOA
#define KEY_1_PIN  GPIO_Pin_8

void     Key_Init(void);
uint32_t Key_Get(void);

#endif
