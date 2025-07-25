#ifndef __BURNER_CONFIG_H__
#define __BURNER_CONFIG_H__

#include "ff.h"
#include "stm32f10x.h"

#define Flash_Path     "0:"                // Flash路径
#define Config_Path    "0:config.json"     // 配置文件路径
#define Firmware_Path  "0:firmware"        // 固件文件路径
#define Readme_Path    "0:readme.txt"      // 说明文件路径
#define Supported_Path "0:supported.txt"   // 支持列表文件路径

#define CONFIG_BUFFER_SIZE 1024   // 配置缓冲区大小

#define CONFIG_BUFFER1_SIZE (CONFIG_BUFFER_SIZE - CONFIG_BUFFER2_SIZE)   // 配置缓冲区1大小
#define CONFIG_BUFFER2_SIZE (128)                                        // 配置缓冲区2大小

#define CONFIG_DEFAULT_AUTO_BURNER     1              // 自动烧录标志
#define CONFIG_DEFAULT_CHIP_ERASE      0              // 擦除全片
#define CONFIG_DEFAULT_READ_PROTECTION 0              // 读保护
#define CONFIG_DEFAULT_AUTO_RUN        1              // 自动运行
#define CONFIG_DEFAULT_VERIFY          0              // 程序校验
#define CONFIG_DEFAULT_FLASH_ADDRESS   "0x08000000"   // 烧录目标地址

typedef struct {
    uint32_t FileAddress;          // 文件地址
    uint32_t FileSize;             // 文件大小
    uint32_t FileCrc;              // 文件CRC32校验码
    uint32_t FlashAddress;         // 烧录起始地址
    char     FilePath[128];        // 文件路径
    uint32_t AutoBurner     : 1;   // 自动烧录标志
    uint32_t ChipErase      : 1;   // 擦除全片
    uint32_t ReadProtection : 1;   // 锁定Flash
    uint32_t AutoRun        : 1;   // 自动运行
    uint32_t Verify         : 1;   // 程序校验
    uint32_t CRC32;                // CRC32校验码
} BurnerConfigInfo_t;

extern BurnerConfigInfo_t BurnerConfigInfo;

void BurnerConfig(void);

#endif   // __BURNER_CONFIG_H__