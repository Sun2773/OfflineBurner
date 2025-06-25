#ifndef __BURNER_CONFIG_H__
#define __BURNER_CONFIG_H__

#include "stm32f10x.h"

#define Flash_Path  "0:"              // Flash·��
#define Config_Path "0:config.json"   // �����ļ�·��

#define CONFIG_BUFFER_SIZE 1024   // ���û�������С

#define FLASH_CONFIG_ADDRESS  0x00000000   // ���ñ����ַ
#define FLASH_PROGRAM_ADDRESS 0x00001000   // ���򱣴��ַ


void BurnerConfig(void);

#endif   // __BURNER_CONFIG_H__