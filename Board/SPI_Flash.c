#include "SPI_Flash.h"
#include "stdio.h"

struct SPI_Flash_Struct {
    uint32_t Capacity;   // Flash容量
} SPI_Flash;

/**
 * @brief  初始化W25Qxx
 * @note
 * @retval None
 */
void W25QXX_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(W25QXX_CS_RCCCLOCK, ENABLE);
    /* 初始化片选引脚 */
    GPIO_InitStructure.GPIO_Pin   = W25QXX_CS_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(W25QXX_CS_PORT, &GPIO_InitStructure);
    /* 默认取消片选 */
    GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
    /* 初始化SPI */
    W25QXX_SPI_Init();
    /* 获取芯片容量 */
    SPI_Flash.Capacity = W25QXX_ReadCapacity();
}

/**
 * @brief  读SR寄存器
 * @note
 * @retval SR寄存器的值
 */
uint8_t W25QXX_ReadSR(void) {
    uint8_t sr = 0;
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送命令:读取SR状态寄存器 */
    W25QXX_ReadWriteByte(W25QX_ReadStatusReg);
    /* 读一个字节 */
    sr = W25QXX_ReadWriteByte(0Xff);
    /* 取消片选 */
    W25QXX_CS_1;
    /* 返回寄存器的值 */
    return sr;
}

/**
 * @brief  W25Qxx 写使能
 * @note
 * @retval None
 */
void W25QXX_Write_Enable(void) {
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送写使能 */
    W25QXX_ReadWriteByte(W25QX_WriteEnable);
    /* 取消片选 */
    W25QXX_CS_1;
}

/**
 * @brief  W25Qxx 写禁止
 * @note
 * @retval None
 */
void W25QXX_Write_Disable(void) {
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送写禁止 */
    W25QXX_ReadWriteByte(W25QX_WriteDisable);
    /* 取消片选 */
    W25QXX_CS_1;
}

/**
 * @brief  W25Qxx 读取芯片ID
 * @note
 * @retval 芯片ID
 */
uint16_t W25QXX_ReadID(void) {
    uint16_t id = 0;
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送读取ID命令 */
    W25QXX_ReadWriteByte(W25QX_ManufactDeviceID);
    /* 等待三个时钟周期 */
    W25QXX_ReadWriteByte(0x00);
    W25QXX_ReadWriteByte(0x00);
    W25QXX_ReadWriteByte(0x00);
    /* 读取两个字节 */
    id |= W25QXX_ReadWriteByte(0xFF) << 8;
    id |= W25QXX_ReadWriteByte(0xFF);
    /* 取消片选 */
    W25QXX_CS_1;
    /* 返回芯片ID */
    return id;
}

/**
 * @brief  读取芯片容量
 * @note
 * @retval 芯片容量(字节)
 */
uint32_t W25QXX_ReadCapacity(void) {
    uint16_t id;
    uint32_t capacity = 1;
    /* 读取芯片制造ID */
    id = W25QXX_ReadID();
    if (id == 0xFFFF) {
        return 0;
    }
    /* 获取容量信息(BCD码) */
    id &= 0x00FF;
    /* 转换次幂 */
    id = ((id >> 4) * 10) + (id & 0x0F);
    /* 计算容量 */
    for (; id; id--) {
        capacity *= 2;
    }
    capacity *= 1024 / 8;
    /* 返回容量 */
    return capacity;
}

/**
 * @brief  直接读取数据
 * @note
 * @param  r_bf: 读取缓冲区
 * @param  r_addr: 读取地址
 * @param  count: 读取字节数
 * @retval None
 */
void W25QXX_Read(void* r_bf, uint32_t r_addr, uint16_t count) {
    uint8_t* r_poi = r_bf;
    /* 等待写入结束 */
    W25QXX_WaitBusy();
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送读取数据命令 */
    W25QXX_ReadWriteByte(W25QX_ReadData);
    /* 发送数据地址 */
    W25QXX_ReadWriteByte((uint8_t) (r_addr >> 16));
    W25QXX_ReadWriteByte((uint8_t) (r_addr >> 8));
    W25QXX_ReadWriteByte((uint8_t) (r_addr));
    /* 循环读取数据 */
    while (count--) {
        *r_poi++ = W25QXX_ReadWriteByte(0XFF);
    }
    /* 取消片选 */
    W25QXX_CS_1;
}

/**
 * @brief  写页 最大256字节
 * @note
 * @param  w_bf: 写入缓冲区
 * @param  w_addr: 写入地址
 * @param  count: 写入字节数
 * @retval None
 */
void W25QXX_WritePage(void* w_bf, uint32_t w_addr, uint16_t count) {
    uint8_t* w_poi = w_bf;
    /* 等待写入结束 */
    W25QXX_WaitBusy();
    /* 写使能 */
    W25QXX_Write_Enable();
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送页编程命令 */
    W25QXX_ReadWriteByte(W25QX_PageProgram);
    /* 发送数据地址 */
    W25QXX_ReadWriteByte((uint8_t) (w_addr >> 16));
    W25QXX_ReadWriteByte((uint8_t) (w_addr >> 8));
    W25QXX_ReadWriteByte((uint8_t) (w_addr));
    /* 循环写入 */
    while (count--) {
        W25QXX_ReadWriteByte(*w_poi++);
    }
    /* 取消片选 */
    W25QXX_CS_1;
    /* 等待写入结束 */
    W25QXX_WaitBusy();
}

/**
 * @brief  直接写入数据 自动换页 无校验
 * @note
 * @param  w_bf: 写入缓冲区
 * @param  w_addr: 写入地址
 * @param  count: 写入字节数
 * @retval None
 */
void W25QXX_Write(void* w_bf, uint32_t w_addr, uint16_t count) {
    uint8_t* w_poi = w_bf;
    uint16_t page_byte;
    /* 写入 */
    while (count) {
        /* 计算当前页剩余空间 */
        page_byte = 256 - w_addr % 256;
        /* 如果剩余写入字节大于当前页剩余空间 */
        if (count > page_byte) {
            /* 写完剩余页 */
            W25QXX_WritePage(w_poi, w_addr, page_byte);
            /* 计算参量 */
            w_poi += page_byte;
            w_addr += page_byte;
            count -= page_byte;
        }
        /* 如果剩余写入字节小于当前页剩余空间 */
        else if (count <= page_byte) {
            /* 写完剩余数据 */
            W25QXX_WritePage(w_poi, w_addr, count);
            /* 计算参量 */
            w_poi += count;
            w_addr += count;
            count -= count;
        }
    }
}

/**
 * @brief  写入数据自动擦除
 * @note
 * @param  w_bf: 写入缓冲区
 * @param  w_addr: 写入地址
 * @param  count: 写入字节数
 * @retval None
 */
void W25QXX_WriteAutoErase(void* w_bf, uint32_t w_addr, uint16_t count) {
    uint32_t sector_addr = 0;
    uint16_t w_cnt       = 0;
    while (count != 0) {
        sector_addr = w_addr & 0xFFFFF000;   // 扇区地址
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
 * @brief  擦除扇区
 * @note
 * @param  address: 擦除地址
 * @retval None
 */
void W25QXX_EraseSector(uint32_t address) {
    /* 整数对齐扇区地址 */
    address /= 4096;
    address *= 4096;
    /* 等待忙位 */
    W25QXX_WaitBusy();
    /* 写使能 */
    W25QXX_Write_Enable();
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送擦除扇区指令 */
    W25QXX_ReadWriteByte(W25QX_SectorErase);
    /* 发送擦除地址 */
    W25QXX_ReadWriteByte((uint8_t) (address >> 16));
    W25QXX_ReadWriteByte((uint8_t) (address >> 8));
    W25QXX_ReadWriteByte((uint8_t) (address));
    /* 取消片选 */
    W25QXX_CS_1;
    /* 等待写入结束 */
    W25QXX_WaitBusy();
}

/**
 * @brief  忙位等待
 * @note
 * @retval None
 */
void W25QXX_WaitBusy(void) {
    while ((W25QXX_ReadSR() & 0x01) == 0x01) {
    }
}

/**
 * @brief  进入掉电模式
 * @note
 * @retval None
 */
void W25QXX_PowerDown(void) {
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送掉电命令 */
    W25QXX_ReadWriteByte(W25QX_PowerDown);
    /* 取消片选 */
    W25QXX_CS_1;
}

/**
 * @brief  唤醒
 * @note
 * @retval None
 */
void W25QXX_WAKEUP(void) {
    /* 片选器件 */
    W25QXX_CS_0;
    /* 发送取消掉电 */
    W25QXX_ReadWriteByte(W25QX_ReleasePowerDown);
    /* 取消片选 */
    W25QXX_CS_1;
}
