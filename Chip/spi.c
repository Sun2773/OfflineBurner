#include "spi.h"

/**
 * @brief  SPI1 ��ʼ��
 * @note
 * @retval None
 */
void SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* SPI��IO�ں�SPI�����ʱ�� */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    /* SPI��IO������ */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;   // ����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                   // ����SPI����ģʽ:����Ϊ��SPI
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;                   // ����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;                      // ����ͬ��ʱ�ӵĿ���״̬Ϊ�͵�ƽ
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;                    // ����ͬ��ʱ�ӵĵ�һ�������أ��������½������ݱ�����
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                      // NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;         // ���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                  // ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                                 // CRCֵ����Ķ���ʽ
    SPI_Init(SPI1, &SPI_InitStructure);                                          // ����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���

    SPI_Cmd(SPI1, ENABLE);      // ʹ��SPI����
    SPI1_ReadWriteByte(0xff);   // ��������
}

/**
 * @brief  SPI1 �ٶ�����
 * @note
 * @param  baud: �ٶ�
 * @retval None
 */
void SPI1_SetSpeed(uint8_t baud) {
    SPI1->CR1 &= 0XFFC7;
    SPI1->CR1 |= baud;
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief  SPI1 ��дһ���ֽ�
 * @note
 * @param  byte: �ֽ�
 * @retval
 */
uint8_t SPI1_ReadWriteByte(uint8_t byte) {
    uint8_t res = 0;
    SPI_Cmd(SPI1, ENABLE);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
    }
    SPI_I2S_SendData(SPI1, byte);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
    }
    res = SPI_I2S_ReceiveData(SPI1);
    SPI_Cmd(SPI1, DISABLE);
    return res;
}

/**
 * @brief  SPI2 ��ʼ��
 * @note
 * @retval None
 */
void SPI2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* SPI��IO�ں�SPI�����ʱ�� */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* SPI��IO������ */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;   // ����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                   // ����SPI����ģʽ:����Ϊ��SPI
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;                   // ����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;                      // ����ͬ��ʱ�ӵĿ���״̬Ϊ�ߵ�ƽ
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;                    // ����ͬ��ʱ�ӵĵڶ��������أ��������½������ݱ�����
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                      // NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;         // ���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                  // ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                                 // CRCֵ����Ķ���ʽ
    SPI_Init(SPI2, &SPI_InitStructure);                                          // ����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���

    SPI_Cmd(SPI2, ENABLE);      // ʹ��SPI����
    SPI2_ReadWriteByte(0xff);   // ��������
}

/**
 * @brief  SPI2 �ٶ�����
 * @note
 * @param  baud: �ٶ�
 * @retval None
 */
void SPI2_SetSpeed(uint8_t baud) {
    SPI2->CR1 &= 0XFFC7;
    SPI2->CR1 |= baud;
    SPI_Cmd(SPI2, ENABLE);
}

/**
 * @brief  SPI2 ��дһ���ֽ�
 * @note
 * @param  byte: �ֽ�
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
 * @brief  SPI3 ��ʼ��
 * @note
 * @retval None
 */
void SPI3_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* SPI��IO�ں�SPI�����ʱ�� */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SPI3, DISABLE);

    /* SPI��IO������ */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;   // ����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                   // ����SPI����ģʽ:����Ϊ��SPI
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;                   // ����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;                     // ����ͬ��ʱ�ӵĿ���״̬Ϊ�ߵ�ƽ
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;                    // ����ͬ��ʱ�ӵĵڶ��������أ��������½������ݱ�����
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                      // NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;         // ���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                  // ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                                 // CRCֵ����Ķ���ʽ
    SPI_Init(SPI3, &SPI_InitStructure);                                          // ����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���

    SPI_Cmd(SPI3, ENABLE);      // ʹ��SPI����
    SPI3_ReadWriteByte(0xff);   // ��������
}

/**
 * @brief  SPI3 �ٶ�����
 * @note
 * @param  baud: �ٶ�
 * @retval None
 */
void SPI3_SetSpeed(uint8_t baud) {
    SPI3->CR1 &= 0XFFC7;
    SPI3->CR1 |= baud;
    SPI_Cmd(SPI3, ENABLE);
}

/**
 * @brief  SPI3 ��дһ���ֽ�
 * @note
 * @param  byte: �ֽ�
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
