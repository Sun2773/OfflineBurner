#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include "spi.h"

// ָ���
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

#define W25QXX_BLOCK_SIZE (4096)

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
 * SPI��Ҫ��ʼ��Ϊ ���е�ƽ�� ��һ�����ز�������
 */
#define W25QXX_SPI_Init() \
    {                     \
        SPI1_Init();      \
    }

#define W25QXX_ReadWriteByte(data) SPI1_ReadWriteByte(data)
#define W25QXX_Bus_Take()          SPI1_Take()
#define W25QXX_Bus_Give()          SPI1_Give()

typedef enum {

    SPI_Flash_WorkState_OK = 0,     // �����ɹ�
    SPI_Flash_WorkState_Busy,       // æ
    SPI_Flash_WorkState_Error,      // ����
    SPI_Flash_WorkState_Idle,       // ����
    SPI_Flash_WorkState_Writing,    // ����д��
    SPI_Flash_WorkState_Reading,    // ���ڶ�ȡ
    SPI_Flash_WorkState_Eraseing,   // ���ڲ���
    SPI_Flash_WorkState_Init,       // ��ʼ״̬

} SPI_FlashWorkState;

#define SPI_FLASH_Init()                     W25QXX_Init()                       // ��ʼ��
#define SPI_FLASH_Write(w_bf, w_addr, count) W25QXX_Write(w_bf, w_addr, count)   // д������
#define SPI_FLASH_Read(r_bf, r_addr, count)  W25QXX_Read(r_bf, r_addr, count)    // ��ȡ����
#define SPI_FLASH_Erase(address)             W25QXX_EraseSector(address)         // ��������

void     W25QXX_Init(void);                                                    // ��ʼ��W25Qxx
uint8_t  W25QXX_ReadSR(void);                                                  // ��SR�Ĵ���
void     W25QXX_Write_Enable(void);                                            // W25Qxx дʹ��
void     W25QXX_Write_Disable(void);                                           // W25Qxx д��ֹ
uint16_t W25QXX_ReadID(void);                                                  // W25Qxx ��ȡоƬID
uint32_t W25QXX_ReadCapacity(void);                                            // ��ȡоƬ����
void     W25QXX_Read(void* r_bf, uint32_t r_addr, uint16_t count);             // ֱ�Ӷ�ȡ����
void     W25QXX_WritePage(void* w_bf, uint32_t w_addr, uint16_t count);        // дҳ ���256�ֽ�
void     W25QXX_Write(void* w_bf, uint32_t w_addr, uint16_t count);            // ֱ��д������ �Զ���ҳ ��У��
void     W25QXX_WriteAutoErase(void* w_bf, uint32_t w_addr, uint16_t count);   // д�������Զ�����
void     W25QXX_EraseSector(uint32_t address);                                 // ��������
void     W25QXX_WaitBusy(void);                                                // æλ�ȴ�
void     W25QXX_PowerDown(void);                                               // �������ģʽ
void     W25QXX_WAKEUP(void);                                                  // ����

#endif
