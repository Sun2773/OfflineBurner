#include "SPI_Flash.h"
#include "stdio.h"

struct SPI_Flash_Struct {
    uint32_t Capacity;   // Flash����
} SPI_Flash;

/**
 * @brief  ��ʼ��W25Qxx
 * @note
 * @retval None
 */
void W25QXX_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    /* ʹ��ʱ�� */
    RCC_APB2PeriphClockCmd(W25QXX_CS_RCCCLOCK, ENABLE);
    /* ��ʼ��Ƭѡ���� */
    GPIO_InitStructure.GPIO_Pin   = W25QXX_CS_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(W25QXX_CS_PORT, &GPIO_InitStructure);
    /* Ĭ��ȡ��Ƭѡ */
    GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
    /* ��ʼ��SPI */
    W25QXX_SPI_Init();
    /* ��ȡоƬ���� */
    SPI_Flash.Capacity = W25QXX_ReadCapacity();
}

/**
 * @brief  ��SR�Ĵ���
 * @note
 * @retval SR�Ĵ�����ֵ
 */
uint8_t W25QXX_ReadSR(void) {
    uint8_t sr = 0;
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ��������:��ȡSR״̬�Ĵ��� */
    W25QXX_ReadWriteByte(W25QX_ReadStatusReg);
    /* ��һ���ֽ� */
    sr = W25QXX_ReadWriteByte(0Xff);
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
    /* ���ؼĴ�����ֵ */
    return sr;
}

/**
 * @brief  W25Qxx дʹ��
 * @note
 * @retval None
 */
void W25QXX_Write_Enable(void) {
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ����дʹ�� */
    W25QXX_ReadWriteByte(W25QX_WriteEnable);
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
}

/**
 * @brief  W25Qxx д��ֹ
 * @note
 * @retval None
 */
void W25QXX_Write_Disable(void) {
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ����д��ֹ */
    W25QXX_ReadWriteByte(W25QX_WriteDisable);
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
}

/**
 * @brief  W25Qxx ��ȡоƬID
 * @note
 * @retval оƬID
 */
uint16_t W25QXX_ReadID(void) {
    uint16_t id = 0;
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ���Ͷ�ȡID���� */
    W25QXX_ReadWriteByte(W25QX_ManufactDeviceID);
    /* �ȴ�����ʱ������ */
    W25QXX_ReadWriteByte(0x00);
    W25QXX_ReadWriteByte(0x00);
    W25QXX_ReadWriteByte(0x00);
    /* ��ȡ�����ֽ� */
    id |= W25QXX_ReadWriteByte(0xFF) << 8;
    id |= W25QXX_ReadWriteByte(0xFF);
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
    /* ����оƬID */
    return id;
}

/**
 * @brief  ��ȡоƬ����
 * @note
 * @retval оƬ����(�ֽ�)
 */
uint32_t W25QXX_ReadCapacity(void) {
    uint16_t id;
    uint32_t capacity = 1;
    /* ��ȡоƬ����ID */
    id = W25QXX_ReadID();
    if (id == 0xFFFF) {
        return 0;
    }
    /* ��ȡ������Ϣ(BCD��) */
    id &= 0x00FF;
    /* ת������ */
    id = ((id >> 4) * 10) + (id & 0x0F);
    /* �������� */
    for (; id; id--) {
        capacity *= 2;
    }
    capacity *= 1024 / 8;
    /* �������� */
    return capacity;
}

/**
 * @brief  ֱ�Ӷ�ȡ����
 * @note
 * @param  r_bf: ��ȡ������
 * @param  r_addr: ��ȡ��ַ
 * @param  count: ��ȡ�ֽ���
 * @retval None
 */
void W25QXX_Read(void* r_bf, uint32_t r_addr, uint16_t count) {
    uint8_t* r_poi = r_bf;
    /* �ȴ�д����� */
    W25QXX_WaitBusy();
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ���Ͷ�ȡ�������� */
    W25QXX_ReadWriteByte(W25QX_ReadData);
    /* �������ݵ�ַ */
    W25QXX_ReadWriteByte((uint8_t) (r_addr >> 16));
    W25QXX_ReadWriteByte((uint8_t) (r_addr >> 8));
    W25QXX_ReadWriteByte((uint8_t) (r_addr));
    /* ѭ����ȡ���� */
    while (count--) {
        *r_poi++ = W25QXX_ReadWriteByte(0XFF);
    }
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
}

/**
 * @brief  дҳ ���256�ֽ�
 * @note
 * @param  w_bf: д�뻺����
 * @param  w_addr: д���ַ
 * @param  count: д���ֽ���
 * @retval None
 */
void W25QXX_WritePage(void* w_bf, uint32_t w_addr, uint16_t count) {
    uint8_t* w_poi = w_bf;
    /* �ȴ�д����� */
    W25QXX_WaitBusy();
    /* дʹ�� */
    W25QXX_Write_Enable();
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ����ҳ������� */
    W25QXX_ReadWriteByte(W25QX_PageProgram);
    /* �������ݵ�ַ */
    W25QXX_ReadWriteByte((uint8_t) (w_addr >> 16));
    W25QXX_ReadWriteByte((uint8_t) (w_addr >> 8));
    W25QXX_ReadWriteByte((uint8_t) (w_addr));
    /* ѭ��д�� */
    while (count--) {
        W25QXX_ReadWriteByte(*w_poi++);
    }
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
    /* �ȴ�д����� */
    W25QXX_WaitBusy();
}

/**
 * @brief  ֱ��д������ �Զ���ҳ ��У��
 * @note
 * @param  w_bf: д�뻺����
 * @param  w_addr: д���ַ
 * @param  count: д���ֽ���
 * @retval None
 */
void W25QXX_Write(void* w_bf, uint32_t w_addr, uint16_t count) {
    uint8_t* w_poi = w_bf;
    uint16_t page_byte;
    /* д�� */
    while (count) {
        /* ���㵱ǰҳʣ��ռ� */
        page_byte = 256 - w_addr % 256;
        /* ���ʣ��д���ֽڴ��ڵ�ǰҳʣ��ռ� */
        if (count > page_byte) {
            /* д��ʣ��ҳ */
            W25QXX_WritePage(w_poi, w_addr, page_byte);
            /* ������� */
            w_poi += page_byte;
            w_addr += page_byte;
            count -= page_byte;
        }
        /* ���ʣ��д���ֽ�С�ڵ�ǰҳʣ��ռ� */
        else if (count <= page_byte) {
            /* д��ʣ������ */
            W25QXX_WritePage(w_poi, w_addr, count);
            /* ������� */
            w_poi += count;
            w_addr += count;
            count -= count;
        }
    }
}

/**
 * @brief  д�������Զ�����
 * @note
 * @param  w_bf: д�뻺����
 * @param  w_addr: д���ַ
 * @param  count: д���ֽ���
 * @retval None
 */
void W25QXX_WriteAutoErase(void* w_bf, uint32_t w_addr, uint16_t count) {
    uint32_t sector_addr = 0;
    uint16_t w_cnt       = 0;
    while (count != 0) {
        sector_addr = w_addr & 0xFFFFF000;   // ������ַ
        if (w_addr == sector_addr) {
            W25QXX_EraseSector(w_addr);
        }
        if ((w_addr - sector_addr) + count > 4096) {
            w_cnt = 4096 - (w_addr - sector_addr);
        } else {
            w_cnt = count;
        }
        W25QXX_Write(w_bf, w_addr, w_cnt);
        count -= w_cnt;
        w_addr += w_cnt;
        w_bf = (void*) ((uint32_t) w_bf + w_cnt);
    }
}

/**
 * @brief  ��������
 * @note
 * @param  address: ������ַ
 * @retval None
 */
void W25QXX_EraseSector(uint32_t address) {
    /* ��������������ַ */
    address /= 4096;
    address *= 4096;
    /* �ȴ�æλ */
    W25QXX_WaitBusy();
    /* дʹ�� */
    W25QXX_Write_Enable();
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ���Ͳ�������ָ�� */
    W25QXX_ReadWriteByte(W25QX_SectorErase);
    /* ���Ͳ�����ַ */
    W25QXX_ReadWriteByte((uint8_t) (address >> 16));
    W25QXX_ReadWriteByte((uint8_t) (address >> 8));
    W25QXX_ReadWriteByte((uint8_t) (address));
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
    /* �ȴ�д����� */
    W25QXX_WaitBusy();
}

/**
 * @brief  æλ�ȴ�
 * @note
 * @retval None
 */
void W25QXX_WaitBusy(void) {
    while ((W25QXX_ReadSR() & 0x01) == 0x01) {
    }
}

/**
 * @brief  �������ģʽ
 * @note
 * @retval None
 */
void W25QXX_PowerDown(void) {
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ���͵������� */
    W25QXX_ReadWriteByte(W25QX_PowerDown);
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
}

/**
 * @brief  ����
 * @note
 * @retval None
 */
void W25QXX_WAKEUP(void) {
    /* Ƭѡ���� */
    W25QXX_CS_0;
    /* ����ȡ������ */
    W25QXX_ReadWriteByte(W25QX_ReleasePowerDown);
    /* ȡ��Ƭѡ */
    W25QXX_CS_1;
}
