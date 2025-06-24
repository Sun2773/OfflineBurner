#include "SWD.h"
#include "string.h"

/*  SWD��ʼ��  */
void SWD_Init(void) {
    /*  �˿ڳ�ʼ��  */
    SWD_Port_Init();
}

/* SWD�ӿڳ�ʼ�� */
void SWD_Port_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    /*  ʱ�ӳ�ʼ��  */
    RCC_APB2PeriphClockCmd(SWD_RCCCLOCK, ENABLE);

    /*  GPIO��ʼ������  */
    GPIO_InitStructure.GPIO_Pin   = SWD_JTCK_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_JTCK_PORT, &GPIO_InitStructure);
    SWD_JTCK_PORT->BSRR = SWD_JTCK_PIN;

    GPIO_InitStructure.GPIO_Pin   = SWD_JTMS_O_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_JTMS_O_PORT, &GPIO_InitStructure);
    SWD_JTMS_O_PORT->BSRR = SWD_JTMS_O_PIN;

    GPIO_InitStructure.GPIO_Pin  = SWD_JTMS_I_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(SWD_JTMS_I_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = SWD_RESET_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_RESET_PORT, &GPIO_InitStructure);
    SWD_RESET_PORT->BSRR = SWD_RESET_PIN;
}

/* SWD��λ */
void SWD_Reset(void) {
    SWD_DATA_MODE_OUT();
    /* ���͸�λ�ź� */
    for (uint8_t i = 0; i < 50; i++) {
        SWD_BIT_SEND(1);
    }
}

/* дDP�Ĵ��� */
SWD_STA SWD_Write_DP(uint8_t addr, uint32_t data) {
    uint8_t head   = 0x81;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    /* д���ַ */
    head |= addr;
    /* ����֡ͷУ��λ */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    /* ��������У��λ */
    for (uint8_t i = 0; i < 32; i++) {
        if ((data << i) & 0x80000000) {
            parity ^= 0x01;
        }
    }
    SWD_DATA_MODE_OUT();
    /* ����֡ͷ */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    /* ��ȡ��λask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    if (ack == SWD_OK) {
        SWD_DATA_MODE_OUT();
        /* �������� */
        for (uint8_t i = 0; i < 32; i++) {
            SWD_BIT_SEND((data >> i) & 0x01);
        }
        /* ������żУ�� */
        SWD_BIT_SEND(parity);
    }
    /* ����Ӧ�� */
    return ack;
}

/* ��DP�Ĵ��� */
SWD_STA SWD_Read_DP(uint8_t addr, uint32_t* data) {
    uint8_t head   = 0xA1;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    *data          = 0x00000000;
    /* д���ַ */
    head |= addr;
    /* ����֡ͷУ��λ */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    SWD_DATA_MODE_OUT();
    /* ����֡ͷ */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    /* ��ȡ��λask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);

    if (ack == SWD_OK) {
        /* �������� */
        for (uint8_t i = 0; i < 32; i++) {
            uint32_t r_data = 0x00000000;
            SWD_BIT_READ(r_data);
            *data |= (r_data << i);
        }
        /* ������żУ�� */
        SWD_BIT_READ(parity);
        uint8_t p = 0;
        /* ��������У��λ */
        for (uint8_t i = 0; i < 32; i++) {
            if (((*data) << i) & 0x80000000) {
                p ^= 0x01;
            }
        }
        if (p != parity) {
            ack = SWD_PARITY_ERROR;
        }
    }
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    /* ����Ӧ�� */
    return ack;
}

/* дAP�Ĵ��� */
SWD_STA SWD_Write_AP(uint8_t addr, uint32_t data) {
    uint8_t head   = 0xC1;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    /* д���ַ */
    head |= addr;
    /* ����֡ͷУ��λ */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    /* ��������У��λ */
    for (uint8_t i = 0; i < 32; i++) {
        if ((data << i) & 0x80000000) {
            parity ^= 0x01;
        }
    }
    SWD_DATA_MODE_OUT();
    /* ����֡ͷ */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    /* ��ȡ��λask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    if (ack == SWD_OK) {
        SWD_DATA_MODE_OUT();
        /* �������� */
        for (uint8_t i = 0; i < 32; i++) {
            SWD_BIT_SEND((data >> i) & 0x01);
        }
        /* ������żУ�� */
        SWD_BIT_SEND(parity);
    }
    /* ����Ӧ�� */
    return ack;
}

/* ��AP�Ĵ��� */
SWD_STA SWD_Read_AP(uint8_t addr, uint32_t* data) {
    uint8_t head   = 0xE1;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    *data          = 0x00000000;
    /* д���ַ */
    head |= addr;
    /* ����֡ͷУ��λ */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    SWD_DATA_MODE_OUT();
    /* ����֡ͷ */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    /* ��ȡ��λask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);

    if (ack == SWD_OK) {
        /* �������� */
        for (uint8_t i = 0; i < 32; i++) {
            uint32_t r_data = 0x00000000;
            SWD_BIT_READ(r_data);
            *data |= (r_data << i);
        }
        /* ������żУ�� */
        SWD_BIT_READ(parity);
        uint8_t p = 0;
        /* ��������У��λ */
        for (uint8_t i = 0; i < 32; i++) {
            if (((*data) << i) & 0x80000000) {
                p ^= 0x01;
            }
        }
        if (p != parity) {
            ack = SWD_PARITY_ERROR;
        }
    }
    /* ��һ��ʱ������ */
    SWD_WAIT_CLOCK();   // Trn
    /* ����Ӧ�� */
    return ack;
}

/* Ŀ���ʼ�� */
SWD_STA SWD_Target_Init(SWD_TargetInfo* info) {
    uint8_t  error    = 0;
    SWD_STA  res      = SWD_ERROR;
    uint32_t ctr_stat = 0;
    memset(info, 0, sizeof(SWD_TargetInfo));

    do {
        SWD_Reset();   // ���߸�λ

        SWD_BYTE_SEND(0x79);   // JTAG->SWD
        SWD_BYTE_SEND(0xE7);

        SWD_BIT_SEND(1);

        SWD_Reset();   // ���߸�λ
        SWD_BIT_SEND(0);
        SWD_BIT_SEND(0);

        res = SWD_Read_DP(SWD_DP0, &info->IDCode);   // ��ȡDP IDCode

        if (res != SWD_OK) {
            continue;
        }

        res = SWD_Write_DP(SWD_DP4, 0x50000000);   // ����AP����

        res = SWD_Read_DP(SWD_DP4, &ctr_stat);

        res = SWD_Write_DP(SWD_DP8, 0x000000F0);   // �趨APBANK
        do {
            res = SWD_Read_AP(SWD_APC, &info->Identification);   // ��һ�ζ�ȡ������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_AP(SWD_APC, &info->Identification);   // ��ȡʶ����
        if (res != SWD_OK) {
            continue;
        }

        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP4, 0x1FFFF7E0);   // �趨��д��ַ
        do {
            res = SWD_Read_AP(SWD_APC, &info->FlashSize);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_AP(SWD_APC, &info->FlashSize);   // ��ȡflash��С
        if (res != SWD_OK) {
            continue;
        }
        info->FlashSize &= 0xFFFF;
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* Ŀ���ֽڶ�д */
SWD_STA SWD_Target_ByteRW(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000000);
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &ap_reg);   // ��һ�ζ�ȡ��������Ч
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* Ŀ����ֶ�д */
SWD_STA SWD_Target_HalfWordRW(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000001);
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &ap_reg);   // ��һ�ζ�ȡ��������Ч
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* Ŀ��ȫ�ֶ�д */
SWD_STA SWD_Target_WordRW(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000002);
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &ap_reg);   // ��һ�ζ�ȡ��������Ч
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* Ŀ�긴λ */
SWD_STA SWD_Target_Reset(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000002);
        res = SWD_Write_AP(SWD_AP4, 0xE000EDFC);
        res = SWD_Write_AP(SWD_APC, 0x05FA0004);   // д��SCB->AIRCR�Ĵ���
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DPC, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* ��һ���Ĵ��� */
SWD_STA SWD_Target_RegisterRead(uint32_t address, uint32_t* data) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP4, address);
        do {
            res = SWD_Read_AP(SWD_APC, &ap_reg);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_AP(SWD_APC, data);   // ��һ�ζ�ȡ��������Ч
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* дһ���Ĵ��� */
SWD_STA SWD_Target_RegisterWrite(uint32_t address, uint32_t data) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // �趨APBANK
        res = SWD_Write_AP(SWD_AP4, address);
        res = SWD_Write_AP(SWD_APC, data);   // д��SCB->AIRCR�Ĵ���
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &dp_reg);   // ��һ�ζ�ȡ��������Ч
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}
