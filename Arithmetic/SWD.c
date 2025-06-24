#include "SWD.h"
#include "string.h"

/*  SWD初始化  */
void SWD_Init(void) {
    /*  端口初始化  */
    SWD_Port_Init();
}

/* SWD接口初始化 */
void SWD_Port_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    /*  时钟初始化  */
    RCC_APB2PeriphClockCmd(SWD_RCCCLOCK, ENABLE);

    /*  GPIO初始化设置  */
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

/* SWD复位 */
void SWD_Reset(void) {
    SWD_DATA_MODE_OUT();
    /* 发送复位信号 */
    for (uint8_t i = 0; i < 50; i++) {
        SWD_BIT_SEND(1);
    }
}

/* 写DP寄存器 */
SWD_STA SWD_Write_DP(uint8_t addr, uint32_t data) {
    uint8_t head   = 0x81;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    /* 写入地址 */
    head |= addr;
    /* 计算帧头校验位 */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    /* 计算数据校验位 */
    for (uint8_t i = 0; i < 32; i++) {
        if ((data << i) & 0x80000000) {
            parity ^= 0x01;
        }
    }
    SWD_DATA_MODE_OUT();
    /* 发送帧头 */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    /* 读取三位ask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    if (ack == SWD_OK) {
        SWD_DATA_MODE_OUT();
        /* 发送数据 */
        for (uint8_t i = 0; i < 32; i++) {
            SWD_BIT_SEND((data >> i) & 0x01);
        }
        /* 发送奇偶校验 */
        SWD_BIT_SEND(parity);
    }
    /* 返回应答 */
    return ack;
}

/* 读DP寄存器 */
SWD_STA SWD_Read_DP(uint8_t addr, uint32_t* data) {
    uint8_t head   = 0xA1;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    *data          = 0x00000000;
    /* 写入地址 */
    head |= addr;
    /* 计算帧头校验位 */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    SWD_DATA_MODE_OUT();
    /* 发送帧头 */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    /* 读取三位ask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);

    if (ack == SWD_OK) {
        /* 接收数据 */
        for (uint8_t i = 0; i < 32; i++) {
            uint32_t r_data = 0x00000000;
            SWD_BIT_READ(r_data);
            *data |= (r_data << i);
        }
        /* 接收奇偶校验 */
        SWD_BIT_READ(parity);
        uint8_t p = 0;
        /* 计算数据校验位 */
        for (uint8_t i = 0; i < 32; i++) {
            if (((*data) << i) & 0x80000000) {
                p ^= 0x01;
            }
        }
        if (p != parity) {
            ack = SWD_PARITY_ERROR;
        }
    }
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    /* 返回应答 */
    return ack;
}

/* 写AP寄存器 */
SWD_STA SWD_Write_AP(uint8_t addr, uint32_t data) {
    uint8_t head   = 0xC1;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    /* 写入地址 */
    head |= addr;
    /* 计算帧头校验位 */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    /* 计算数据校验位 */
    for (uint8_t i = 0; i < 32; i++) {
        if ((data << i) & 0x80000000) {
            parity ^= 0x01;
        }
    }
    SWD_DATA_MODE_OUT();
    /* 发送帧头 */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    /* 读取三位ask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    if (ack == SWD_OK) {
        SWD_DATA_MODE_OUT();
        /* 发送数据 */
        for (uint8_t i = 0; i < 32; i++) {
            SWD_BIT_SEND((data >> i) & 0x01);
        }
        /* 发送奇偶校验 */
        SWD_BIT_SEND(parity);
    }
    /* 返回应答 */
    return ack;
}

/* 读AP寄存器 */
SWD_STA SWD_Read_AP(uint8_t addr, uint32_t* data) {
    uint8_t head   = 0xE1;
    SWD_STA ack    = SWD_ERROR;
    uint8_t parity = 0x00;
    *data          = 0x00000000;
    /* 写入地址 */
    head |= addr;
    /* 计算帧头校验位 */
    for (uint8_t i = 0; i < 4; i++) {
        if ((head << i) & 0x40) {
            head ^= 0x04;
        }
    }
    SWD_DATA_MODE_OUT();
    /* 发送帧头 */
    for (uint8_t i = 0; i < 8; i++) {
        SWD_BIT_SEND((head << i) & 0x80);
    }
    SWD_DATA_MODE_IN();
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    /* 读取三位ask */
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);
    ack <<= 1;
    SWD_BIT_READ(ack);

    if (ack == SWD_OK) {
        /* 接收数据 */
        for (uint8_t i = 0; i < 32; i++) {
            uint32_t r_data = 0x00000000;
            SWD_BIT_READ(r_data);
            *data |= (r_data << i);
        }
        /* 接收奇偶校验 */
        SWD_BIT_READ(parity);
        uint8_t p = 0;
        /* 计算数据校验位 */
        for (uint8_t i = 0; i < 32; i++) {
            if (((*data) << i) & 0x80000000) {
                p ^= 0x01;
            }
        }
        if (p != parity) {
            ack = SWD_PARITY_ERROR;
        }
    }
    /* 空一个时钟周期 */
    SWD_WAIT_CLOCK();   // Trn
    /* 返回应答 */
    return ack;
}

/* 目标初始化 */
SWD_STA SWD_Target_Init(SWD_TargetInfo* info) {
    uint8_t  error    = 0;
    SWD_STA  res      = SWD_ERROR;
    uint32_t ctr_stat = 0;
    memset(info, 0, sizeof(SWD_TargetInfo));

    do {
        SWD_Reset();   // 总线复位

        SWD_BYTE_SEND(0x79);   // JTAG->SWD
        SWD_BYTE_SEND(0xE7);

        SWD_BIT_SEND(1);

        SWD_Reset();   // 总线复位
        SWD_BIT_SEND(0);
        SWD_BIT_SEND(0);

        res = SWD_Read_DP(SWD_DP0, &info->IDCode);   // 读取DP IDCode

        if (res != SWD_OK) {
            continue;
        }

        res = SWD_Write_DP(SWD_DP4, 0x50000000);   // 启用AP调试

        res = SWD_Read_DP(SWD_DP4, &ctr_stat);

        res = SWD_Write_DP(SWD_DP8, 0x000000F0);   // 设定APBANK
        do {
            res = SWD_Read_AP(SWD_APC, &info->Identification);   // 第一次读取数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_AP(SWD_APC, &info->Identification);   // 读取识别码
        if (res != SWD_OK) {
            continue;
        }

        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP4, 0x1FFFF7E0);   // 设定读写地址
        do {
            res = SWD_Read_AP(SWD_APC, &info->FlashSize);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_AP(SWD_APC, &info->FlashSize);   // 读取flash大小
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

/* 目标字节读写 */
SWD_STA SWD_Target_ByteRW(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000000);
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &ap_reg);   // 第一次读取的数据无效
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* 目标半字读写 */
SWD_STA SWD_Target_HalfWordRW(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000001);
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &ap_reg);   // 第一次读取的数据无效
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* 目标全字读写 */
SWD_STA SWD_Target_WordRW(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000002);
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &ap_reg);   // 第一次读取的数据无效
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* 目标复位 */
SWD_STA SWD_Target_Reset(void) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP0, 0x23000002);
        res = SWD_Write_AP(SWD_AP4, 0xE000EDFC);
        res = SWD_Write_AP(SWD_APC, 0x05FA0004);   // 写入SCB->AIRCR寄存器
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DPC, &dp_reg);   // 第一次读取的数据无效
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* 读一个寄存器 */
SWD_STA SWD_Target_RegisterRead(uint32_t address, uint32_t* data) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t ap_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP4, address);
        do {
            res = SWD_Read_AP(SWD_APC, &ap_reg);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_AP(SWD_APC, data);   // 第一次读取的数据无效
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}

/* 写一个寄存器 */
SWD_STA SWD_Target_RegisterWrite(uint32_t address, uint32_t data) {
    uint8_t  error  = 0;
    SWD_STA  res    = SWD_ERROR;
    uint32_t dp_reg = 0;
    do {
        res = SWD_Write_DP(SWD_DP8, 0x00000000);   // 设定APBANK
        res = SWD_Write_AP(SWD_AP4, address);
        res = SWD_Write_AP(SWD_APC, data);   // 写入SCB->AIRCR寄存器
        do {
            res = SWD_Read_DP(SWD_DPC, &dp_reg);   // 第一次读取的数据无效
        } while (res == SWD_Wait);
        res = SWD_Read_DP(SWD_DP4, &dp_reg);   // 第一次读取的数据无效
        break;
    } while (++error < 5);

    if (error >= 5) {
        res = SWD_ERROR;
    } else {
        res = SWD_OK;
    }

    return res;
}
