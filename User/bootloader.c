#include "stm32f10x.h"

#include "FlashLayout.h"
#include "Tool.h"
#include "stdio.h"

#include "SPI_Flash.h"
#include "led.h"

#define FLASH_SECTOR(addr) (addr & ~0x3FF)

typedef void (*pFunction)(void);   // 用户程序跳转函数类型声明
pFunction Jump_To_App;             // 用户程序跳转函数指针

uint8_t Buffer[1024];   // 缓存区

uint8_t        IAP_JumpApp(void);
static uint8_t SPI_Flash_2_Flash(uint32_t spi_f_addr, uint32_t f_addr, uint32_t size);

/* *
 * @brief  主函数
 * @param  None
 * @retval None
 */
int main(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);   // 使能PWR和BKP外设时钟
    PWR_BackupAccessCmd(ENABLE);

    uint32_t addr = 0;   //
    uint32_t size = 0;   //

    addr = BKP_ReadBackupRegister(BKP_DR1);
    addr <<= 16;
    addr |= BKP_ReadBackupRegister(BKP_DR2);

    size = BKP_ReadBackupRegister(BKP_DR3);
    size <<= 16;
    size |= BKP_ReadBackupRegister(BKP_DR4);

    BKP_WriteBackupRegister(BKP_DR1, 0x0000);   // 清除BKP寄存器
    BKP_WriteBackupRegister(BKP_DR2, 0x0000);   // 清除BKP寄存器
    BKP_WriteBackupRegister(BKP_DR3, 0x0000);   // 清除BKP寄存器
    BKP_WriteBackupRegister(BKP_DR4, 0x0000);   // 清除BKP寄存器

    if (addr != SPI_FLASH_FIRMWARE_ADDRESS || size == 0) {
        IAP_JumpApp();
    }

    /* 外设初始化 */
    LED_Init();      // 初始化LED
    W25QXX_Init();   // 初始化SPI Flash

    SPI_Flash_2_Flash(SPI_FLASH_FIRMWARE_ADDRESS, CHIP_FIRMWARE_ADDRESS, size);
    /* 复位 */
    NVIC_SystemReset();
    while (1) {
    }
}

static uint8_t SPI_Flash_2_Flash(uint32_t flash_addr, uint32_t chip_addr, uint32_t size) {
    /* 解锁内部Flash */
    FLASH_Unlock();
    /* 设置延时两个周期 */
    FLASH_SetLatency(FLASH_Latency_2);

    uint32_t finish = 0;   // 已完成大小
    uint32_t r_cnt;        // 读取计数
    /* 复制文件 */
    while (size - finish) {
        /* 检查剩余字节数,若剩余字节大于缓存 */
        if ((size - finish) > 1024) {
            r_cnt = 1024;
        }
        /* 剩余字节数大于0小于缓存 */
        else if ((size - finish) > 0) {
            r_cnt = (size - finish);
        }
        /* 读取剩余字节数 */
        W25QXX_Read(Buffer, flash_addr, r_cnt);
        flash_addr += r_cnt;   // Flash地址累计
        /* 擦除扇区 */
        if (FLASH_ErasePage(FLASH_SECTOR(chip_addr)) != FLASH_COMPLETE) {   // 一定要判断是否擦除成功
            return 1;
        }
        /* 将数据写入Flash */
        for (uint16_t i = 0; i < r_cnt;) {
            uint16_t word = 0;
            /* 每两个个字节组合成一个半字(2byte) */
            word = ((uint32_t) Buffer[i++]);
            word |= ((uint32_t) Buffer[i++]) << 8;
            /* 将字写入Flash */
            FLASH_ProgramHalfWord(chip_addr, word);
            /* Flash地址累计 */
            chip_addr += 2;
        }
        /* 计数 */
        finish += r_cnt;
        LED_OnOff(ERR);
    }

    /* Flash上锁 */
    FLASH_Lock();
    return 0;
}

/* 跳转到用户程序 */
uint8_t IAP_JumpApp(void) {
    /* 检查是否有应用程序 */
    if (((*(__IO uint32_t*) CHIP_FIRMWARE_ADDRESS) & 0x2FFE0000) == 0x20000000) {
        /* 设置中断向量表指向 */
        NVIC_SetVectorTable(NVIC_VectTab_FLASH, CHIP_FIRMWARE_ADDRESS);
        /* 跳转函数指向用户应用程序地址 */
        Jump_To_App = (pFunction) (*(__IO uint32_t*) (CHIP_FIRMWARE_ADDRESS + 4));
        /* 初始化用户程序堆栈指针 */
        __set_MSP(*(__IO uint32_t*) CHIP_FIRMWARE_ADDRESS);
        /* 跳转用户程序 */
        Jump_To_App();
    }
    return 1;
}

#ifdef USE_FULL_ASSERT

/* *
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1) {
    }
}
#endif
