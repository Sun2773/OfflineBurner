#ifndef __SPI_H__
#define __SPI_H__

#include "stm32f10x.h"

void    SPI1_Init(void);                      // 初始化SPI1口
void    SPI1_SetSpeed(uint8_t SpeedSet);      // 设置SPI1速度
uint8_t SPI1_ReadWriteByte(uint8_t TxData);   // SPI1总线读写一个字节

void    SPI2_Init(void);                      // 初始化SPI2口
void    SPI2_SetSpeed(uint8_t SpeedSet);      // 设置SPI2速度
uint8_t SPI2_ReadWriteByte(uint8_t TxData);   // SPI2总线读写一个字节

void    SPI3_Init(void);                      // 初始化SPI3口
void    SPI3_SetSpeed(uint8_t SpeedSet);      // 设置SPI3速度
uint8_t SPI3_ReadWriteByte(uint8_t TxData);   // SPI3总线读写一个字节

#endif
