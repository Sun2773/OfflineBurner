#include "BurnerConfig.h"

#include <stdlib.h>
#include "FlashLayout.h"
#include "SPI_Flash.h"
#include "Version.h"
#include "cJSON.h"
#include "crc.h"
#include "ff.h"
#include "heap.h"
#include "led.h"
#include "string.h"

BurnerConfigInfo_t BurnerConfigInfo = {
    .FilePath   = "",
    .AutoBurner = 1,
    .ChipErase  = 0,
    .ChipLock   = 0,
    .AutoRun    = 1,
};

void BurnerConfig(void) {
    FATFS*   fs          = NULL;    // 文件系统对象
    FIL*     file        = NULL;    // 文件对象
    FILINFO* file_info   = NULL;    // 文件信息对象
    FRESULT  f_res       = FR_OK;   // FATFS操作结果
    char*    str_buf     = NULL;    // 字符串缓冲区
    UINT     r_cnt       = 0;       // 读取结果
    uint32_t config_flag = 0;       // 配置标志
    uint32_t crc         = 0;       // CRC校验码

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

    /********************************* 读取系统配置 *********************************/
    SPI_FLASH_Read(str_buf,
                   SPI_FLASH_CONFIG_ADDRESS,
                   sizeof(BurnerConfigInfo_t));
    /* 检查CRC32校验 */
    crc = CRC32_Update(0, str_buf, sizeof(BurnerConfigInfo_t) - 4);
    if (crc == ((BurnerConfigInfo_t*) str_buf)->CRC32) {
        /* CRC校验失败，清空配置 */
        memcpy(&BurnerConfigInfo, str_buf, sizeof(BurnerConfigInfo_t));
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
                r_cnt = CONFIG_BUFFER_SIZE;
            } else {
                /* 剩余字节数大于0小于缓存,读取剩余字节数 */
                r_cnt = (file_size - file_finish);
            }
            /* 读取文件 */
            if (f_read(file, str_buf, r_cnt, &r_cnt) != FR_OK) {
                break;
            }
            rw_addr = SPI_FLASH_FIRMWARE_ADDRESS + file_finish;   // 写入地址
            if ((rw_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* 如果写入地址是块大小的整数倍，擦除块 */
                SPI_FLASH_Erase(rw_addr);
            }
            SPI_FLASH_Write(str_buf, rw_addr, r_cnt);
            /* 计数 */
            file_finish += r_cnt;
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
                r_cnt = CONFIG_BUFFER_SIZE;
            } else {
                /* 剩余字节数大于0小于缓存,读取剩余字节数 */
                r_cnt = (file_size - file_finish);
            }
            /* 读取文件 */
            if (f_read(file, str_buf, r_cnt, &r_cnt) != FR_OK) {
                break;
            }

            /* 计算文件校验码 */
            file_crc32 = CRC32_Update(file_crc32, str_buf, r_cnt);
            rw_addr    = SPI_FLASH_PROGRAM_ADDRESS + file_finish;   // 写入地址
            if ((rw_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* 如果写入地址是块大小的整数倍，擦除块 */
                SPI_FLASH_Erase(rw_addr);
                LED_OnOff(ERR);
            }
            SPI_FLASH_Write(str_buf, rw_addr, r_cnt);
            /* 计数 */
            file_finish += r_cnt;
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
                r_cnt = CONFIG_BUFFER_SIZE;
            }
            /* 剩余字节数大于0小于缓存 */
            else {
                /* 读取剩余字节数 */
                r_cnt = (file_size - file_finish);
            }
            rw_addr = SPI_FLASH_PROGRAM_ADDRESS + file_finish;   // 读取地址
            /* 读取数据 */
            SPI_FLASH_Read(str_buf, rw_addr, r_cnt);
            /* 计算数据校验码 */
            data_crc32 = CRC32_Update(data_crc32, str_buf, r_cnt);
            /* 计数 */
            file_finish += r_cnt;
            LED_OnOff(RUN);
        }
        /* 校验数据 */
        if (data_crc32 == file_crc32) {
            BurnerConfigInfo.FileAddress = SPI_FLASH_PROGRAM_ADDRESS;   // 获取文件地址
            BurnerConfigInfo.FileSize    = file_finish;                 // 获取文件大小
            BurnerConfigInfo.FileCrc     = file_crc32;                  // 更新文件CRC32校验码
            BurnerConfigInfo.Flag        = 0;                           // 更新配置标志
            /* 删除文件 */
            f_res = f_unlink(BurnerConfigInfo.FilePath);
        } else {
        }
    } while (0);
    LED_Off(RUN);
    LED_Off(ERR);

    /********************************* 检查配置文件 *********************************/
    do {
        if ((f_res = f_stat(Config_Path, file_info)) == FR_OK) {
            config_flag = ((uint32_t) (file_info->fdate) << 16) | file_info->ftime;
        }
        /* 配置文件发生变化，读取配置文件 */
        if (config_flag != BurnerConfigInfo.Flag || f_res == FR_NO_FILE) {
            if ((f_res = f_open(file, Config_Path, FA_READ | FA_WRITE | FA_OPEN_ALWAYS)) != FR_OK) {
                goto ex;
            }
            // 如果文件不存在，创建一个默认配置
            uint32_t file_size = f_size(file);

            if ((file_size != 0) && (file_size < CONFIG_BUFFER_SIZE)) {
                f_res = f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt);
            }

            cJSON* root = NULL;   // JSON根对象
            cJSON* item = NULL;   // JSON项

            root = cJSON_ParseWithLength(str_buf, r_cnt);   // 解析JSON字符串
            if (root == NULL) {
                root = cJSON_CreateObject();   // 创建一个新的JSON对象
            }

            if ((item = cJSON_GetObjectItem(root, "file")) != NULL) {
                cJSON_SetValuestring(item, BurnerConfigInfo.FilePath);
            } else {
                cJSON_AddStringToObject(root, "file", BurnerConfigInfo.FilePath);
            }
            if ((item = cJSON_GetObjectItem(root, "autoBurn")) != NULL) {
                if (cJSON_IsTrue(item)) {
                    BurnerConfigInfo.AutoBurner = 1;
                } else {
                    BurnerConfigInfo.AutoBurner = 0;
                }
            } else {
                if (BurnerConfigInfo.AutoBurner) {
                    cJSON_AddTrueToObject(root, "autoBurn");
                } else {
                    cJSON_AddFalseToObject(root, "autoBurn");
                }
            }
            if ((item = cJSON_GetObjectItem(root, "chipErase")) != NULL) {
                if (cJSON_IsTrue(item)) {
                    BurnerConfigInfo.ChipErase = 1;
                } else {
                    BurnerConfigInfo.ChipErase = 0;
                }
            } else {
                if (BurnerConfigInfo.ChipErase) {
                    cJSON_AddTrueToObject(root, "chipErase");
                } else {
                    cJSON_AddFalseToObject(root, "chipErase");
                }
            }
            if ((item = cJSON_GetObjectItem(root, "chipLock")) != NULL) {
                if (cJSON_IsTrue(item)) {
                    BurnerConfigInfo.ChipLock = 1;
                } else {
                    BurnerConfigInfo.ChipLock = 0;
                }
            } else {
                if (BurnerConfigInfo.ChipLock) {
                    cJSON_AddTrueToObject(root, "chipLock");
                } else {
                    cJSON_AddFalseToObject(root, "chipLock");
                }
            }
            if ((item = cJSON_GetObjectItem(root, "autoRun")) != NULL) {
                if (cJSON_IsTrue(item)) {
                    BurnerConfigInfo.AutoRun = 1;
                } else {
                    BurnerConfigInfo.AutoRun = 0;
                }
            } else {
                if (BurnerConfigInfo.AutoRun) {
                    cJSON_AddTrueToObject(root, "autoRun");
                } else {
                    cJSON_AddFalseToObject(root, "autoRun");
                }
            }
            if ((item = cJSON_GetObjectItem(root, "version")) != NULL) {
                cJSON_SetValuestring(item, SYSTEM_VERSION);
            } else {
                cJSON_AddStringToObject(root, "version", SYSTEM_VERSION);
            }
            char* json = cJSON_PrintUnformatted(root);   // 将JSON对象转换为字符串
            cJSON_Delete(root);                          // 删除JSON对象

            f_res = f_lseek(file, 0);
            f_res = f_write(file, json, strlen(json), &r_cnt);
            f_res = f_truncate(file);
            vPortFree(json);

            f_close(file);

            f_res       = f_stat(Config_Path, file_info);
            config_flag = ((uint32_t) (file_info->fdate) << 16) | file_info->ftime;
        }
    } while (0);

    /********************************* 更新配置 *********************************/
    if (config_flag != BurnerConfigInfo.Flag) {
        BurnerConfigInfo.Flag  = config_flag;
        BurnerConfigInfo.CRC32 = CRC32_Update(0, &BurnerConfigInfo, sizeof(BurnerConfigInfo) - 4);

        SPI_FLASH_Erase(SPI_FLASH_CONFIG_ADDRESS);
        SPI_FLASH_Write(&BurnerConfigInfo,
                        SPI_FLASH_CONFIG_ADDRESS,
                        sizeof(BurnerConfigInfo));
    }
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
