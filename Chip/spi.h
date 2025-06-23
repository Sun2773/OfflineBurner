#ifndef __SPI_H__
#define __SPI_H__

#include "stm32f10x.h"

void    SPI1_Init(void);                      // ��ʼ��SPI1��
void    SPI1_SetSpeed(uint8_t SpeedSet);      // ����SPI1�ٶ�
uint8_t SPI1_ReadWriteByte(uint8_t TxData);   // SPI1���߶�дһ���ֽ�

void    SPI2_Init(void);                      // ��ʼ��SPI2��
void    SPI2_SetSpeed(uint8_t SpeedSet);      // ����SPI2�ٶ�
uint8_t SPI2_ReadWriteByte(uint8_t TxData);   // SPI2���߶�дһ���ֽ�

void    SPI3_Init(void);                      // ��ʼ��SPI3��
void    SPI3_SetSpeed(uint8_t SpeedSet);      // ����SPI3�ٶ�
uint8_t SPI3_ReadWriteByte(uint8_t TxData);   // SPI3���߶�дһ���ֽ�

#endif
