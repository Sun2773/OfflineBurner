#ifndef __BURNER_CONFIG_H__
#define __BURNER_CONFIG_H__

#include "stm32f10x.h"

#define Flash_Path  "0:"              // Flash路径
#define Config_Path "0:config.json"   // 配置文件路径

#define CONFIG_BUFFER_SIZE 1024   // 配置缓冲区大小

#define FLASH_CONFIG_ADDRESS  0x00000000   // 配置保存地址
#define FLASH_PROGRAM_ADDRESS 0x00001000   // 程序保存地址

typedef struct {
    uint32_t Flag;            // Flash大小
    uint32_t FileAddress;     // 文件地址
    uint32_t FileSize;        // 文件大小
    uint32_t FileCrc;         // 文件CRC32校验码
    char     FilePath[128];   // 文件路径
} BurnerConfigInfo_t;

extern BurnerConfigInfo_t BurnerConfigInfo;

void BurnerConfig(void);

#endif   // __BURNER_CONFIG_H__