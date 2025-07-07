#include "spi.h"

/**
 * @brief  SPI1 初始化
 * @note
 * @retval None
 */
void SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* SPI的IO口和SPI外设打开时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    /* SPI的IO口设置 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;   // 设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                   // 设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;                   // 设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;                      // 串行同步时钟的空闲状态为低电平
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;                    // 串行同步时钟的第一个跳变沿（上升或下降）数据被采样
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                      // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;           // 定义波特率预分频的值:波特率预分频值为2
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                  // 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                                 // CRC值计算的多项式
    SPI_Init(SPI1, &SPI_InitStructure);                                          // 根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

    SPI_Cmd(SPI1, ENABLE);      // 使能SPI外设
    SPI1_ReadWriteByte(0xff);   // 启动传输
}

/**
 * @brief  SPI1 速度设置
 * @note
 * @param  baud: 速度
 * @retval None
 */
void SPI1_SetSpeed(uint8_t baud) {
    SPI1->CR1 &= 0XFFC7;
    SPI1->CR1 |= baud;
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief  SPI1 读写一个字节
 * @note
 * @param  byte: 字节
 * @retval
 */
uint8_t SPI1_ReadWriteByte(uint8_t byte) {
    uint8_t res = 0;
    uint16_t timeout = 0x7FF;
    SPI_Cmd(SPI1, ENABLE);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET && timeout--) {
    }
    SPI_I2S_SendData(SPI1, byte);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET && timeout--) {
    }
    res = SPI_I2S_ReceiveData(SPI1);
    SPI_Cmd(SPI1, DISABLE);
    return res;
}

/**
 * @brief  SPI2 初始化
 * @note
 * @retval None
 */
void SPI2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* SPI的IO口和SPI外设打开时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* SPI的IO口设置 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;   // 设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                   // 设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;                   // 设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;                      // 串行同步时钟的空闲状态为高电平
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;                    // 串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                      // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;         // 定义波特率预分频的值:波特率预分频值为256
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                  // 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                                 // CRC值计算的多项式
    SPI_Init(SPI2, &SPI_InitStructure);                                          // 根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

    SPI_Cmd(SPI2, ENABLE);      // 使能SPI外设
    SPI2_ReadWriteByte(0xff);   // 启动传输
}

/**
 * @brief  SPI2 速度设置
 * @note
 * @param  baud: 速度
 * @retval None
 */
void SPI2_SetSpeed(uint8_t baud) {
    SPI2->CR1 &= 0XFFC7;
    SPI2->CR1 |= baud;
    SPI_Cmd(SPI2, ENABLE);
}

/**
 * @brief  SPI2 读写一个字节
 * @note
 * @param  byte: 字节
 * @retval
 */
uint8_t SPI2_ReadWriteByte(uint8_t byte) {
    uint8_t res = 0;
    SPI_Cmd(SPI2, ENABLE);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) {
    }
    SPI_I2S_SendData(SPI2, byte);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) {
    }
    res = SPI_I2S_ReceiveData(SPI2);
    SPI_Cmd(SPI2, DISABLE);
    return res;
}

/**
 * @brief  SPI3 初始化
 * @note
 * @retval None
 */
void SPI3_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* SPI的IO口和SPI外设打开时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SPI3, DISABLE);

    /* SPI的IO口设置 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;   // 设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                   // 设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;                   // 设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;                     // 串行同步时钟的空闲状态为高电平
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;                    // 串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                      // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;         // 定义波特率预分频的值:波特率预分频值为256
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                  // 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                                 // CRC值计算的多项式
    SPI_Init(SPI3, &SPI_InitStructure);                                          // 根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

    SPI_Cmd(SPI3, ENABLE);      // 使能SPI外设
    SPI3_ReadWriteByte(0xff);   // 启动传输
}

/**
 * @brief  SPI3 速度设置
 * @note
 * @param  baud: 速度
 * @retval None
 */
void SPI3_SetSpeed(uint8_t baud) {
    SPI3->CR1 &= 0XFFC7;
    SPI3->CR1 |= baud;
    SPI_Cmd(SPI3, ENABLE);
}

/**
 * @brief  SPI3 读写一个字节
 * @note
 * @param  byte: 字节
 * @retval
 */
uint8_t SPI3_ReadWriteByte(uint8_t byte) {
    uint8_t res = 0;
    SPI_Cmd(SPI3, ENABLE);
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) {
    }
    SPI_I2S_SendData(SPI3, byte);
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET) {
    }
    res = SPI_I2S_ReceiveData(SPI3);
    SPI_Cmd(SPI3, DISABLE);
    return res;
}
