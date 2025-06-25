#include "BurnerConfig.h"

#include <stdlib.h>
#include "SPI_Flash.h"
#include "Version.h"
#include "cJSON.h"
#include "crc.h"
#include "ff.h"
#include "heap.h"
#include "led.h"
#include "string.h"

BurnerConfigInfo_t BurnerConfigInfo = {
    .FilePath = "",
};

void BurnerConfig(void) {
    FATFS*   fs          = NULL;    // 文件系统对象
    FIL*     file        = NULL;    // 文件对象
    FILINFO* file_info   = NULL;    // 文件信息对象
    FRESULT  f_res       = FR_OK;   // FATFS操作结果
    char*    str_buf     = NULL;    // 字符串缓冲区
    UINT     r_cnt       = 0;       // 读取结果
    uint32_t config_flag = 0;       // 配置标志
    /* 读取Flash配置 */
    SPI_FLASH_Read(&BurnerConfigInfo,
                   FLASH_CONFIG_ADDRESS,
                   sizeof(BurnerConfigInfo));
    config_flag = BurnerConfigInfo.Flag;   // 获取配置标志
    /* 如果配置标志为0xFFFFFFFF，表示没有配置 */
    if (config_flag == 0xFFFFFFFF) {
        memset(&BurnerConfigInfo, 0, sizeof(BurnerConfigInfo));
    }
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
            rw_addr    = FLASH_PROGRAM_ADDRESS + file_finish;   // 写入地址
            if ((rw_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* 如果写入地址是块大小的整数倍，擦除块 */
                SPI_FLASH_Erase(rw_addr);
            }
            SPI_FLASH_Write(str_buf, rw_addr, r_cnt);
            /* 计数 */
            file_finish += r_cnt;
            LED_OnOff(RUN);
        }
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
            rw_addr = FLASH_PROGRAM_ADDRESS + file_finish;   // 读取地址
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
            BurnerConfigInfo.FileAddress = FLASH_PROGRAM_ADDRESS;   // 获取文件地址
            BurnerConfigInfo.FileSize    = file_finish;             // 获取文件大小
            BurnerConfigInfo.FileCrc     = file_crc32;              // 更新文件CRC32校验码
            BurnerConfigInfo.Flag        = 0;                       // 更新配置标志
            /* 删除文件 */
            f_res = f_unlink(BurnerConfigInfo.FilePath);
        } else {
        }
    } while (0);

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
            if ((item = cJSON_GetObjectItem(root, "version")) != NULL) {
                cJSON_SetValuestring(item, SYSTEM_VERSION);
            } else {
                cJSON_AddStringToObject(root, "version", SYSTEM_VERSION);
            }
            str_buf = cJSON_PrintUnformatted(root);   // 将JSON对象转换为字符串
            cJSON_Delete(root);                       // 删除JSON对象

            f_res = f_lseek(file, 0);   // 将文件指针移动到文件开头
            f_res = f_write(file, str_buf, strlen(str_buf), &r_cnt);
            f_res = f_truncate(file);   // 截断文件到当前大小

            f_close(file);

            f_res       = f_stat(Config_Path, file_info);
            config_flag = ((uint32_t) (file_info->fdate) << 16) | file_info->ftime;
        }
    } while (0);
    if (config_flag != BurnerConfigInfo.Flag) {
        BurnerConfigInfo.Flag = config_flag;
        SPI_FLASH_Erase(FLASH_CONFIG_ADDRESS);   // 擦除Flash配置地址
        SPI_FLASH_Write(&BurnerConfigInfo,
                        FLASH_CONFIG_ADDRESS,
                        sizeof(BurnerConfigInfo));   // 写入Flash配置
    }
ex:
    if (file != NULL) {
        f_close(file);
        vPortFree(file);
    }
    /*取消挂载*/
    f_res = f_mount(0, "0:", 1);

    if (fs != NULL) {
        vPortFree(fs);
    }
    if (str_buf != NULL) {
        vPortFree(str_buf);
    }
    return;
}