#include "BurnerConfig.h"

#include <stdlib.h>
#include "FlashLayout.h"
#include "SPI_Flash.h"
#include "Version.h"
#include "cJSON.h"
#include "crc.h"
#include "heap.h"
#include "led.h"
#include "string.h"

#include "ConfigReadme.h"

static uint8_t HEX2DEC(char* hex, uint32_t* dec);

BurnerConfigInfo_t BurnerConfigInfo = {
    .FilePath   = "",
    .AutoBurner = 1,
    .ChipErase  = 0,
    .ChipLock   = 0,
    .AutoRun    = 1,
};

void BurnerConfig(void) {
    FATFS*   fs        = NULL;    // 文件系统对象
    FIL*     file      = NULL;    // 文件对象
    FILINFO* file_info = NULL;    // 文件信息对象
    FRESULT  f_res     = FR_OK;   // FATFS操作结果
    char*    str_buf   = NULL;    // 字符串缓冲区
    UINT     rw_cnt    = 0;       // 读取结果
    uint32_t crc       = 0;       // CRC校验码

    if ((fs = pvPortMalloc(sizeof(FATFS))) == NULL) {
        goto ex;
    }
    if ((file_info = pvPortMalloc(sizeof(FILINFO))) == NULL) {
        goto ex;
    }
    if ((file = pvPortMalloc(sizeof(FIL))) == NULL) {
        goto ex;
    }
    if ((str_buf = pvPortMalloc(CONFIG_BUFFER_SIZE)) == NULL) {
        goto ex;
    }

    /********************************* 挂载文件系统 *********************************/
    /* 挂载文件体统 */
    f_res = f_mount(fs, Flash_Path, 1);
    /* 没有文件系统 */
    if (f_res == FR_NO_FILESYSTEM) {
        /* 格式化Flash */
        f_res = f_mkfs("0:", 0, NULL, 4096);
        /* 取消挂载 */
        f_res = f_mount(0, "0:", 1);
        /* 再次挂载 */
        f_res = f_mount(fs, "0:", 1);
    }
    /* 挂载失败 */
    if (f_res != FR_OK) {
        goto ex;
    }

    /********************************* 检查是否需要固件升级 *********************************/
    do {
        DIR f_dp = {0};   // 目录对象
        /* 打开固件目录 */
        if ((f_res = f_opendir(&f_dp, Firmware_Path)) != FR_OK) {
            break;
        }
        /* 寻找固件文件 */
        while (1) {
            /* 读取文件 */
            f_readdir(&f_dp, file_info);
            /* 无文件结束 */
            if (*file_info->fname == '\0') {
                break;
            }
            /* 排除文件夹 */
            if ((file_info->fattrib & AM_DIR) != 0) {
                continue;
            }
            /* 判断是否为固件文件 */
            if (((memcmp(strrchr(file_info->fname, '.'), ".bin", 4) == 0) ||
                 (memcmp(strrchr(file_info->fname, '.'), ".BIN", 4) == 0))) {
                break;
            }
        }

        /* 如果没有找到固件文件 */
        if (*file_info->fname == '\0') {
            break;
        }
        strcpy(str_buf, Firmware_Path);      // 复制固件路径
        strcat(str_buf, "/");                // 追加斜杠
        strcat(str_buf, file_info->fname);   // 追加文件名
        /* 打开固件文件 */
        if ((f_res = f_open(file, str_buf, FA_READ)) != FR_OK) {
            break;
        }
        uint32_t file_size   = f_size(file);   // 文件大小
        uint32_t file_finish = 0;              // 已完成大小
        uint32_t rw_addr     = 0;              // 读写地址
        LED_Off(RUN);
        LED_Off(ERR);
        /* 开始复制文件 */
        while (file_size - file_finish) {
            if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
                /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
                rw_cnt = CONFIG_BUFFER_SIZE;
            } else {
                /* 剩余字节数大于0小于缓存,读取剩余字节数 */
                rw_cnt = (file_size - file_finish);
            }
            /* 读取文件 */
            if (f_read(file, str_buf, rw_cnt, &rw_cnt) != FR_OK) {
                break;
            }
            rw_addr = SPI_FLASH_FIRMWARE_ADDRESS + file_finish;   // 写入地址
            if ((rw_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* 如果写入地址是块大小的整数倍，擦除块 */
                SPI_FLASH_Erase(rw_addr);
            }
            SPI_FLASH_Write(str_buf, rw_addr, rw_cnt);
            /* 计数 */
            file_finish += rw_cnt;
            LED_OnOff(RUN);
            LED_OnOff(ERR);
        }
        /* 删除文件 */
        f_res = f_del(Firmware_Path);
        BKP_WriteBackupRegister(BKP_DR1, SPI_FLASH_FIRMWARE_ADDRESS >> 16);      //
        BKP_WriteBackupRegister(BKP_DR2, SPI_FLASH_FIRMWARE_ADDRESS & 0xFFFF);   //
        BKP_WriteBackupRegister(BKP_DR3, file_finish >> 16);                     //
        BKP_WriteBackupRegister(BKP_DR4, file_finish & 0xFFFF);                  //
        LED_Off(RUN);
        LED_Off(ERR);
        NVIC_SystemReset();   // 重启系统
    } while (0);

    /********************************* 读取系统配置 *********************************/
    SPI_FLASH_Read(str_buf,
                   SPI_FLASH_CONFIG_ADDRESS,
                   sizeof(BurnerConfigInfo_t));
    /* 检查CRC32校验 */
    crc = CRC32_Update(0, str_buf, sizeof(BurnerConfigInfo_t) - 4);
    if (crc == ((BurnerConfigInfo_t*) str_buf)->CRC32) {
        /* CRC校验成功，使用配置 */
        memcpy(&BurnerConfigInfo, str_buf, sizeof(BurnerConfigInfo_t));
    }

    /********************************* 检查是否需要加载程序 *********************************/
    do {
        DIR f_dp = {0};   // 目录对象
        /* 打开目录 */
        if ((f_res = f_opendir(&f_dp, Flash_Path)) != FR_OK) {
            goto ex;
        }
        /* 寻找固件文件 */
        while (1) {
            /* 读取文件 */
            f_readdir(&f_dp, file_info);
            /* 无文件结束 */
            if (*file_info->fname == '\0') {
                break;
            }
            /* 排除文件夹 */
            if ((file_info->fattrib & AM_DIR) != 0) {
                continue;
            }
            /* 判断是否为固件文件 */
            if (((memcmp(strrchr(file_info->fname, '.'), ".bin", 4) == 0) ||
                 (memcmp(strrchr(file_info->fname, '.'), ".BIN", 4) == 0))) {
                break;
            }
        }

        /* 如果没有找到固件文件 */
        if (*file_info->fname == '\0') {
            break;
        }
        *BurnerConfigInfo.FilePath = '\0';                     // 清空文件路径
        strcat(BurnerConfigInfo.FilePath, Flash_Path);         // 复制Flash路径
        strcat(BurnerConfigInfo.FilePath, file_info->fname);   // 追加文件名
        /* 打开固件文件 */
        if ((f_res = f_open(file, BurnerConfigInfo.FilePath, FA_READ)) != FR_OK) {
            break;
        }
        uint32_t file_size   = f_size(file);   // 文件大小
        uint32_t file_finish = 0;              // 已完成大小
        uint32_t rw_addr     = 0;              // 读写地址
        uint32_t data_crc32  = 0;              // 数据校验码
        uint32_t file_crc32  = 0;              // 文件校验码
        LED_Off(RUN);
        LED_Off(ERR);
        /* 开始复制文件 */
        while (file_size - file_finish) {
            if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
                /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
                rw_cnt = CONFIG_BUFFER_SIZE;
            } else {
                /* 剩余字节数大于0小于缓存,读取剩余字节数 */
                rw_cnt = (file_size - file_finish);
            }
            /* 读取文件 */
            if (f_read(file, str_buf, rw_cnt, &rw_cnt) != FR_OK) {
                break;
            }

            /* 计算文件校验码 */
            file_crc32 = CRC32_Update(file_crc32, str_buf, rw_cnt);
            rw_addr    = SPI_FLASH_PROGRAM_ADDRESS + file_finish;   // 写入地址
            if ((rw_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* 如果写入地址是块大小的整数倍，擦除块 */
                SPI_FLASH_Erase(rw_addr);
                LED_OnOff(ERR);
            }
            SPI_FLASH_Write(str_buf, rw_addr, rw_cnt);
            /* 计数 */
            file_finish += rw_cnt;
            LED_OnOff(RUN);
        }
        LED_Off(RUN);
        LED_Off(ERR);
        /* 重新赋值 */
        file_size   = file_finish;
        file_finish = 0;
        /* 开始校验文件 */
        while (file_size - file_finish) {
            /* 检查剩余字节数,若剩余字节大于缓存 */
            if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
                /* 读取缓存大小文件 */
                rw_cnt = CONFIG_BUFFER_SIZE;
            }
            /* 剩余字节数大于0小于缓存 */
            else {
                /* 读取剩余字节数 */
                rw_cnt = (file_size - file_finish);
            }
            rw_addr = SPI_FLASH_PROGRAM_ADDRESS + file_finish;   // 读取地址
            /* 读取数据 */
            SPI_FLASH_Read(str_buf, rw_addr, rw_cnt);
            /* 计算数据校验码 */
            data_crc32 = CRC32_Update(data_crc32, str_buf, rw_cnt);
            /* 计数 */
            file_finish += rw_cnt;
            LED_OnOff(RUN);
        }
        /* 校验数据 */
        if (data_crc32 == file_crc32) {
            BurnerConfigInfo.FileAddress = SPI_FLASH_PROGRAM_ADDRESS;   // 获取文件地址
            BurnerConfigInfo.FileSize    = file_finish;                 // 获取文件大小
            BurnerConfigInfo.FileCrc     = file_crc32;                  // 更新文件CRC32校验码
            /* 删除文件 */
            f_res = f_unlink(BurnerConfigInfo.FilePath);
        } else {
        }
    } while (0);
    LED_Off(RUN);
    LED_Off(ERR);

    /********************************* 检查配置文件 *********************************/
    {
        if ((f_res = f_open(file, Config_Path, FA_READ | FA_WRITE | FA_OPEN_ALWAYS)) != FR_OK) {
            goto ex;
        }
        // 如果文件不存在，创建一个默认配置
        uint32_t file_size = f_size(file);

        if ((file_size != 0) && (file_size < CONFIG_BUFFER_SIZE)) {
            f_res = f_read(file, str_buf, CONFIG_BUFFER_SIZE, &rw_cnt);
        }
        crc = CRC32_Update(0, str_buf, rw_cnt);   // 计算CRC32校验码

        cJSON* root = NULL;   // JSON根对象
        cJSON* item = NULL;   // JSON项

        root = cJSON_ParseWithLength(str_buf, rw_cnt);   // 解析JSON字符串
        if (root == NULL) {
            root = cJSON_CreateObject();   // 创建一个新的JSON对象
        }
        /* 保存烧录文件名 */
        if ((item = cJSON_GetObjectItem(root, "file")) != NULL) {
            cJSON_SetValuestring(item, BurnerConfigInfo.FilePath);
        } else {
            cJSON_AddStringToObject(root, "file", BurnerConfigInfo.FilePath);
        }

/* 处理对象中的数据变量 */
#define CONFIG_OBJECT_INT(father, name, class, variable, default) \
    {                                                             \
        if ((item = cJSON_GetObjectItem(father, name)) != NULL) { \
            if (cJSON_IsNumber(item)) {                           \
                variable = (class) cJSON_GetNumberValue(item);    \
            } else {                                              \
                cJSON_DeleteItemFromObject(father, name);         \
                cJSON_AddNumberToObject(father, name, default);   \
                variable = default;                               \
            }                                                     \
        } else {                                                  \
            cJSON_AddNumberToObject(father, name, default);       \
            variable = default;                                   \
        }                                                         \
    }
        /* 更新配置项 */
        CONFIG_OBJECT_INT(root, "autoBurn", uint8_t, BurnerConfigInfo.AutoBurner, CONFIG_DEFAULT_AUTO_BURNER);
        CONFIG_OBJECT_INT(root, "chipErase", uint8_t, BurnerConfigInfo.ChipErase, CONFIG_DEFAULT_CHIP_ERASE);
        CONFIG_OBJECT_INT(root, "chipLock", uint8_t, BurnerConfigInfo.ChipLock, CONFIG_DEFAULT_CHIP_LOCK);
        CONFIG_OBJECT_INT(root, "autoRun", uint8_t, BurnerConfigInfo.AutoRun, CONFIG_DEFAULT_AUTO_RUN);
        /* 烧录地址 */
        if ((item = cJSON_GetObjectItem(root, "flashAddr")) != NULL) {
            if (cJSON_IsString(item)) {
                char* flash_addr = cJSON_GetStringValue(item);
                if (HEX2DEC(flash_addr, &BurnerConfigInfo.FlashAddress) == 0) {
                    cJSON_SetValuestring(item, CONFIG_DEFAULT_FLASH_ADDRESS);
                    HEX2DEC(CONFIG_DEFAULT_FLASH_ADDRESS, &BurnerConfigInfo.FlashAddress);
                }
            } else {
                cJSON_DeleteItemFromObject(root, "flashAddr");
                cJSON_AddStringToObject(root, "flashAddr", CONFIG_DEFAULT_FLASH_ADDRESS);
                HEX2DEC(CONFIG_DEFAULT_FLASH_ADDRESS, &BurnerConfigInfo.FlashAddress);
            }
        } else {
            cJSON_AddStringToObject(root, "flashAddr", CONFIG_DEFAULT_FLASH_ADDRESS);
            HEX2DEC(CONFIG_DEFAULT_FLASH_ADDRESS, &BurnerConfigInfo.FlashAddress);
        }
        /* 更新版本信息 */
        if ((item = cJSON_GetObjectItem(root, "version")) != NULL) {
            cJSON_SetValuestring(item, SYSTEM_VERSION);
        } else {
            cJSON_AddStringToObject(root, "version", SYSTEM_VERSION);
        }

        char* json = cJSON_PrintUnformatted(root);   // 将JSON对象转换为字符串
        if (crc != CRC32_Update(0, json, strlen(json))) {
            f_res = f_lseek(file, 0);
            f_res = f_write(file, json, strlen(json), &rw_cnt);
            f_res = f_truncate(file);
        }
        cJSON_Delete(root);
        vPortFree(json);
        f_close(file);
    }

    /********************************* 更新配置 *********************************/
    crc = CRC32_Update(0, &BurnerConfigInfo, sizeof(BurnerConfigInfo) - 4);
    if (crc != BurnerConfigInfo.CRC32) {
        BurnerConfigInfo.CRC32 = crc;
        SPI_FLASH_Erase(SPI_FLASH_CONFIG_ADDRESS);
        SPI_FLASH_Write(&BurnerConfigInfo,
                        SPI_FLASH_CONFIG_ADDRESS,
                        sizeof(BurnerConfigInfo));
    }

    /********************************* 检查说明文件 *********************************/
    if ((f_res = f_open(file, Readme_Path, FA_WRITE | FA_READ | FA_OPEN_ALWAYS)) != FR_OK) {
        goto ex;
    }
    if ((f_res = f_read(file, str_buf, CONFIG_BUFFER_SIZE, &rw_cnt)) != FR_OK) {
        goto ex;
    }
    crc = CRC32_Update(0, str_buf, rw_cnt);   // 计算CRC32校验码
    if (crc != CRC32_Update(0, (void*) ConfigReadme, strlen(ConfigReadme))) {
        f_res = f_lseek(file, 0);
        f_res = f_write(file, ConfigReadme, strlen(ConfigReadme), &rw_cnt);
        f_res = f_truncate(file);
    }
    f_close(file);

ex:
    if (file != NULL) {
        f_close(file);
        vPortFree(file);
    }
    /* 取消挂载 */
    f_res = f_mount(0, "0:", 1);

    if (fs != NULL) {
        vPortFree(fs);
    }
    if (str_buf != NULL) {
        vPortFree(str_buf);
    }
    return;
}

static uint8_t HEX2DEC(char* hex, uint32_t* dec) {
    if (hex == NULL || dec == NULL) {
        return 0;   // 参数错误
    }
    uint32_t value = 0;
    if (hex[0] == '0' && ((hex[1] == 'x') || (hex[1] == 'X'))) {
        for (uint8_t i = 2; (hex[i] != '\0') && (i < 10); i++) {
            if (hex[i] >= '0' && hex[i] <= '9') {
                value <<= 4;
                value |= (hex[i] - '0');
            } else if (hex[i] >= 'A' && hex[i] <= 'F') {
                value <<= 4;
                value |= (hex[i] - 'A' + 10);
            } else if (hex[i] >= 'a' && hex[i] <= 'f') {
                value <<= 4;
                value |= (hex[i] - 'a' + 10);
            } else {
                return 0;   // 非法字符
            }
        }
        *dec = value;
        return 1;   // 成功转换
    }
    return 0;   // 非法格式
}
