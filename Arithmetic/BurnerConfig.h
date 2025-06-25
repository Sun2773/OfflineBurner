#ifndef __BURNER_CONFIG_H__
#define __BURNER_CONFIG_H__

#include "stm32f10x.h"

#define Flash_Path  "0:"              // Flash路径
#define Config_Path "0:config.json"   // 配置文件路径

#define CONFIG_BUFFER_SIZE 1024   // 配置缓冲区大小

#define FLASH_CONFIG_ADDRESS  0x00000000   // 配置保存地址
#define FLASH_PROGRAM_ADDRESS 0x00001000   // 程序保存地址


void BurnerConfig(void);

#endif   // __BURNER_CONFIG_H__