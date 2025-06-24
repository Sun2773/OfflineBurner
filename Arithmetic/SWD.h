#ifndef __SWD_H__
#define __SWD_H__

#include "stm32f10x.h"

/* 接口定义 */

/* ******************************************** */
/*  主机端口  主机引脚    目标端口    目标引脚  */
/*  JTCK      (PB13)  ->  SWCLK       (PA14)    */
/*  JTMS_I    (PB14)  ->  SWDIO       (PA13)    */
/*  JTMS_O    (PB12)  ->  SWDIO       (PA13)    */
/*  RESET     (PB0)   ->  NRST        (NRST)    */
/* ******************************************** */

/*  SWD接口时钟 */
#define SWD_RCCCLOCK RCC_APB2Periph_GPIOB
/*  时钟引脚  */
#define SWD_JTCK_PORT GPIOB
#define SWD_JTCK_PIN  GPIO_Pin_13
/*  数据输入引脚  */
#define SWD_JTMS_I_PORT GPIOB
#define SWD_JTMS_I_PIN  GPIO_Pin_14
/*  数据输出引脚  */
#define SWD_JTMS_O_PORT GPIOB
#define SWD_JTMS_O_PIN  GPIO_Pin_12
/*  复位引脚  */
#define SWD_RESET_PORT GPIOB
#define SWD_RESET_PIN  GPIO_Pin_0

/*  功能定义  */

/* 四分之一时钟 */
#define SWD_THIRD_CLOCK()                            \
    for (__IO uint8_t delay = 3; delay > 0; delay--) \
        ;   // 1us
/* 数据输出模式 */
#define SWD_DATA_MODE_OUT()                  \
    {                                        \
        uint32_t reg = SWD_JTMS_O_PORT->CRL; \
        reg &= ~0x000F0000;                  \
        reg |= 0x00030000;                   \
        SWD_JTMS_O_PORT->CRL = reg;          \
    }
/* 数据输入模式 */
#define SWD_DATA_MODE_IN()                   \
    {                                        \
        uint32_t reg = SWD_JTMS_O_PORT->CRL; \
        reg &= ~0x000F0000;                  \
        reg |= 0x00040000;                   \
        SWD_JTMS_O_PORT->CRL = reg;          \
    }
/* 数据位输出 */
#define SWD_BIT_OUT(bit) ((bit != 0) ? (SWD_JTMS_O_PORT->BSRR = SWD_JTMS_O_PIN) : (SWD_JTMS_O_PORT->BRR = SWD_JTMS_O_PIN))
/* 数据位输入 */
#define SWD_BIT_IN() ((SWD_JTMS_I_PORT->IDR & SWD_JTMS_I_PIN) ? 1 : 0)
/* 时钟输出高 */
#define SWD_CLK_H() (SWD_JTCK_PORT->BSRR = SWD_JTCK_PIN)
/* 时钟输出低 */
#define SWD_CLK_L() (SWD_JTCK_PORT->BRR = SWD_JTCK_PIN)
/* 复位输出高 */
#define SWD_RST_H() (SWD_RESET_PORT->BSRR = SWD_RESET_PIN)
/* 复位输出低 */
#define SWD_RST_L() (SWD_RESET_PORT->BRR = SWD_RESET_PIN)

/* 发送一位 */
#define SWD_BIT_SEND(bit)   \
    {                       \
        SWD_CLK_H();        \
        SWD_THIRD_CLOCK();  \
        SWD_CLK_L();        \
        SWD_BIT_OUT((bit)); \
        SWD_THIRD_CLOCK();  \
        SWD_CLK_H();        \
        SWD_THIRD_CLOCK();  \
    }
/* 读取一位 */
#define SWD_BIT_READ(bit)    \
    {                        \
        SWD_CLK_H();         \
        SWD_THIRD_CLOCK();   \
        SWD_CLK_L();         \
        SWD_THIRD_CLOCK();   \
        bit |= SWD_BIT_IN(); \
        SWD_CLK_H();         \
        SWD_THIRD_CLOCK();   \
    }

/* 等待一个时钟 */
#define SWD_WAIT_CLOCK()   \
    {                      \
        SWD_CLK_H();       \
        SWD_THIRD_CLOCK(); \
        SWD_CLK_L();       \
        SWD_THIRD_CLOCK(); \
        SWD_CLK_H();       \
        SWD_THIRD_CLOCK(); \
    }

/* 等待一个时钟周期 */
#define SWD_WAIT_CYCLE()   \
    {                      \
        SWD_THIRD_CLOCK(); \
        SWD_THIRD_CLOCK(); \
        SWD_THIRD_CLOCK(); \
    }

#define SWD_BYTE_SEND(byte)               \
    for (uint8_t i = 0; i < 8; i++) {     \
        SWD_BIT_SEND(byte & (0x80 >> i)); \
    }

/* DP寄存器地址 */
#define SWD_DP0 0x00
#define SWD_DP4 0x10
#define SWD_DP8 0x08
#define SWD_DPC 0x18

/* AP寄存器地址 */
#define SWD_AP0 0x00
#define SWD_AP4 0x10
#define SWD_AP8 0x08
#define SWD_APC 0x18

/* SWD总线状态 */
typedef enum {

    SWD_ERROR        = 0x00,
    SWD_Fail         = 0x01,
    SWD_Wait         = 0x02,
    SWD_OK           = 0x04,
    SWD_NOREPLY      = 0x07,
    SWD_PARITY_ERROR = 0x08,

} SWD_STA;

typedef struct {
    uint32_t IDCode;           //
    uint32_t Identification;   //
    uint32_t FlashSize;        //

} SWD_TargetInfo;

/* SWD初始化 */
void SWD_Init(void);
/* SWD接口初始化 */
void SWD_Port_Init(void);
/* SWD复位 */
void SWD_Reset(void);
/* 写DP寄存器 */
SWD_STA SWD_Write_DP(uint8_t addr, uint32_t data);
/* 读DP寄存器 */
SWD_STA SWD_Read_DP(uint8_t addr, uint32_t* data);
/* 写AP寄存器 */
SWD_STA SWD_Write_AP(uint8_t addr, uint32_t data);
/* 读AP寄存器 */
SWD_STA SWD_Read_AP(uint8_t addr, uint32_t* data);
/* 目标初始化 */
SWD_STA SWD_Target_Init(SWD_TargetInfo* info);
/* 目标复位 */
SWD_STA SWD_Target_Reset(void);
/* 目标字节读写 */
SWD_STA SWD_Target_ByteRW(void);
/* 目标半字读写 */
SWD_STA SWD_Target_HalfWordRW(void);
/* 目标全字读写 */
SWD_STA SWD_Target_WordRW(void);
/* 读一个寄存器 */
SWD_STA SWD_Target_RegisterRead(uint32_t address, uint32_t* data);
/* 写一个寄存器 */
SWD_STA SWD_Target_RegisterWrite(uint32_t address, uint32_t data);

#endif
