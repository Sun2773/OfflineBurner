#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include "spi.h"

// 指令表
#define W25QX_WriteEnable      0x06
#define W25QX_WriteDisable     0x04
#define W25QX_ReadStatusReg    0x05
#define W25QX_WriteStatusReg   0x01
#define W25QX_ReadData         0x03
#define W25QX_FastReadData     0x0B
#define W25QX_FastReadDual     0x3B
#define W25QX_PageProgram      0x02
#define W25QX_BlockErase       0xD8
#define W25QX_SectorErase      0x20
#define W25QX_ChipErase        0xC7
#define W25QX_PowerDown        0xB9
#define W25QX_ReleasePowerDown 0xAB
#define W25QX_DeviceID         0xAB
#define W25QX_ManufactDeviceID 0x90
#define W25QX_JedecDeviceID    0x9F

#define W25QXX_BLOCK_SIZE (0x1000)   // 4K

#define W25QXX_CS_RCCCLOCK RCC_APB2Periph_GPIOA
#define W25QXX_CS_PORT     GPIOA
#define W25QXX_CS_PIN      GPIO_Pin_4

#define W25QXX_CS_0                                    \
    {                                                  \
        GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN); \
    }
#define W25QXX_CS_1                                  \
    {                                                \
        GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN); \
    }

/**
 * SPI需要初始化为 空闲电平低 第一跳变沿采样数据
 */
#define W25QXX_SPI_Init() \
    {                     \
        SPI1_Init();      \
    }

#define W25QXX_ReadWriteByte(data) SPI1_ReadWriteByte(data)
#define W25QXX_Bus_Take()          SPI1_Take()
#define W25QXX_Bus_Give()          SPI1_Give()

typedef enum {

    SPI_Flash_WorkState_OK = 0,     // 操作成功
    SPI_Flash_WorkState_Busy,       // 忙
    SPI_Flash_WorkState_Error,      // 错误
    SPI_Flash_WorkState_Idle,       // 空闲
    SPI_Flash_WorkState_Writing,    // 正在写入
    SPI_Flash_WorkState_Reading,    // 正在读取
    SPI_Flash_WorkState_Eraseing,   // 正在擦除
    SPI_Flash_WorkState_Init,       // 初始状态

} SPI_FlashWorkState;

#define SPI_FLASH_Init()                     W25QXX_Init()                       // 初始化
#define SPI_FLASH_Write(w_bf, w_addr, count) W25QXX_Write(w_bf, w_addr, count)   // 写入数据
#define SPI_FLASH_Read(r_bf, r_addr, count)  W25QXX_Read(r_bf, r_addr, count)    // 读取数据
#define SPI_FLASH_Erase(address)             W25QXX_EraseSector(address)         // 擦除扇区

void     W25QXX_Init(void);                                                    // 初始化W25Qxx
uint8_t  W25QXX_ReadSR(void);                                                  // 读SR寄存器
void     W25QXX_Write_Enable(void);                                            // W25Qxx 写使能
void     W25QXX_Write_Disable(void);                                           // W25Qxx 写禁止
uint16_t W25QXX_ReadID(void);                                                  // W25Qxx 读取芯片ID
uint32_t W25QXX_ReadCapacity(void);                                            // 读取芯片容量
void     W25QXX_Read(void* r_bf, uint32_t r_addr, uint16_t count);             // 直接读取数据
void     W25QXX_WritePage(void* w_bf, uint32_t w_addr, uint16_t count);        // 写页 最大256字节
void     W25QXX_Write(void* w_bf, uint32_t w_addr, uint16_t count);            // 直接写入数据 自动换页 无校验
void     W25QXX_WriteAutoErase(void* w_bf, uint32_t w_addr, uint16_t count);   // 写入数据自动擦除
void     W25QXX_EraseSector(uint32_t address);                                 // 擦除扇区
void     W25QXX_WaitBusy(void);                                                // 忙位等待
void     W25QXX_PowerDown(void);                                               // 进入掉电模式
void     W25QXX_WAKEUP(void);                                                  // 唤醒

#endif
