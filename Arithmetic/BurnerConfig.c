#include "BurnerConfig.h"

#include "FlashLayout.h"
#include "SPI_Flash.h"
#include "Tool.h"
#include "Version.h"
#include "cJSON.h"
#include "crc.h"
#include "flash_blob.h"
#include "heap.h"
#include "led.h"
#include "stdio.h"
#include "stdlib.h"
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

/**
 * @brief  烧录配置
 * @note
 * @retval None
 */
void BurnerConfig(void) {
    FATFS*   fs                 = NULL;    // 文件系统对象
    FIL*     file               = NULL;    // 文件对象
    FILINFO* file_info          = NULL;    // 文件信息对象
    FRESULT  f_res              = FR_OK;   // FATFS操作结果
    char*    str_buf            = NULL;    // 字符串缓冲区
    UINT     r_cnt              = 0;       // 读取结果
    uint32_t w_addr             = 0;       // 读写地址
    uint32_t crc                = 0;       // CRC校验码
    uint8_t  burner_addr_update = 0;       // 烧录地址更新标志

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);   // 使能PWR和BKP外设时钟
    PWR_BackupAccessCmd(ENABLE);

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
        if (f_opendir(&f_dp, Firmware_Path) != FR_OK) {
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

        /* 取消只读属性 */
        if ((file_info->fattrib & AM_RDO) != 0) {
            f_chmod(str_buf, file_info->fattrib & ~AM_RDO, AM_RDO);
        }
        /* 打开固件文件 */
        if (f_open(file, str_buf, FA_READ) != FR_OK) {
            break;
        }
        uint32_t prog_size = 0;   // 已完成大小
        LED_Off(RUN);
        LED_Off(ERR);
        /* 开始复制文件 */
        while ((f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt) == FR_OK) &&
               (r_cnt != 0)) {
            w_addr = SPI_FLASH_FIRMWARE_ADDRESS + prog_size;   // 写入地址
            if ((w_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* 如果写入地址是块大小的整数倍，擦除块 */
                SPI_FLASH_Erase(w_addr);
            }
            SPI_FLASH_Write(str_buf, w_addr, r_cnt);
            /* 计数 */
            prog_size += r_cnt;
            LED_OnOff(RUN);
            LED_OnOff(ERR);
        }
        /* 删除文件 */
        f_res = f_del(Firmware_Path);
        BKP_WriteBackupRegister(BKP_DR1, SPI_FLASH_FIRMWARE_ADDRESS >> 16);      //
        BKP_WriteBackupRegister(BKP_DR2, SPI_FLASH_FIRMWARE_ADDRESS & 0xFFFF);   //
        BKP_WriteBackupRegister(BKP_DR3, prog_size >> 16);                       //
        BKP_WriteBackupRegister(BKP_DR4, prog_size & 0xFFFF);                    //
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
        if (f_opendir(&f_dp, Flash_Path) != FR_OK) {
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
            if ((memcmp(strrchr(file_info->fname, '.'), ".bin", 4) == 0) ||
                (memcmp(strrchr(file_info->fname, '.'), ".BIN", 4) == 0) ||
                (memcmp(strrchr(file_info->fname, '.'), ".hex", 4) == 0) ||
                (memcmp(strrchr(file_info->fname, '.'), ".HEX", 4) == 0)) {
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

        /* 取消只读属性 */
        if ((file_info->fattrib & AM_RDO) != 0) {
            f_chmod(BurnerConfigInfo.FilePath, file_info->fattrib & ~AM_RDO, AM_RDO);
        }
        /* 打开固件文件 */
        if (f_open(file, BurnerConfigInfo.FilePath, FA_READ) != FR_OK) {
            break;
        }
        uint32_t prog_size = 0;   // 已完成大小

        LED_Off(RUN);
        LED_Off(ERR);
        /* 判断是否为bin文件 */
        if ((memcmp(strrchr(BurnerConfigInfo.FilePath, '.'), ".bin", 4) == 0) ||
            (memcmp(strrchr(BurnerConfigInfo.FilePath, '.'), ".BIN", 4) == 0)) {
            /* 开始复制文件 */
            while ((f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt) == FR_OK) &&
                   (r_cnt != 0)) {
                w_addr = SPI_FLASH_PROGRAM_ADDRESS + prog_size;   // 写入地址
                if ((w_addr % W25QXX_BLOCK_SIZE) == 0) {
                    /* 如果写入地址是块大小的整数倍，擦除块 */
                    SPI_FLASH_Erase(w_addr);
                    LED_OnOff(ERR);
                }
                SPI_FLASH_Write(str_buf, w_addr, r_cnt);
                /* 计数 */
                prog_size += r_cnt;
                LED_OnOff(RUN);
            }
        } /* 判断是否为hex文件 */
        else if ((memcmp(strrchr(BurnerConfigInfo.FilePath, '.'), ".hex", 4) == 0) ||
                 (memcmp(strrchr(BurnerConfigInfo.FilePath, '.'), ".HEX", 4) == 0)) {
            uint32_t start_addr = 0xFFFFFFFF;   // 起始地址
            uint32_t ext_addr   = 0;            // 扩展地址
            uint32_t seg_addr   = 0;            // 段地址
            uint8_t  end_flag   = 0;            // 结束标志
            char*    r_poi      = str_buf;      // 读取指针
            char*    end        = NULL;         // 结束符指针
            char*    poi        = NULL;         // 缓冲区处理指针

            uint8_t* w_buf    = pvPortMalloc(W25QXX_BLOCK_SIZE);   // 写入缓冲区
            uint32_t buf_addr = 0xFFFFFFFF;                        // 写入缓冲区地址
            uint32_t w_flag[24];                                   // 写入标志 3M/4K/32bit
            memset(w_flag, 0, sizeof(w_flag));                     // 初始化写入标志
            prog_size = 0;

#define FLASH_W_FLAG(addr) BIT_VAL(w_flag[((addr - SPI_FLASH_PROGRAM_ADDRESS) / W25QXX_BLOCK_SIZE) / 32], (addr - SPI_FLASH_PROGRAM_ADDRESS) / W25QXX_BLOCK_SIZE % 32)

#define FLASH_BUF_SYNC()                                  \
    if (buf_addr != 0xFFFFFFFF) {                         \
        FLASH_W_FLAG(buf_addr) = 1;                       \
        W25QXX_EraseSector(buf_addr);                     \
        W25QXX_Write(w_buf, buf_addr, W25QXX_BLOCK_SIZE); \
        buf_addr = 0xFFFFFFFF;                            \
    }
            r_cnt = CONFIG_BUFFER1_SIZE;   // 文件读取计数
            while ((end_flag == 0) &&
                   (f_read(file, r_poi, r_cnt, &r_cnt) == FR_OK) &&
                   (r_cnt != 0)) {
                r_poi[r_cnt] = '\0';      // 添加字符串结束符
                poi          = str_buf;   // 重新开始处理数据
                while (1) {
                    if (((poi = strchr(poi, ':')) == NULL) ||    // 查找换行符
                        ((end = strchr(poi, '\n')) == NULL)) {   // 查找结束符
                        /* 当前缓冲区内没有完整报文 */
                        uint16_t remain = CONFIG_BUFFER1_SIZE - (r_poi - str_buf);   // 计算缓冲区中剩余数据
                        memcpy(str_buf, r_poi, remain);                              // 将剩余数据移动到缓冲区开头
                        r_poi = str_buf + remain;                                    // 读取指针更新
                        r_cnt = CONFIG_BUFFER1_SIZE - remain;                        // 计算下次读取的字节数
                        break;                                                       //
                    }

                    uint8_t*  buf    = (uint8_t*) (str_buf + CONFIG_BUFFER1_SIZE);   // 字符串缓冲区指针
                    uint8_t*  head   = (uint8_t*) (buf);                             // 帧头
                    uint8_t*  len    = (uint8_t*) (head + 1);                        // 数据长度
                    uint16_t* offset = (uint16_t*) (len + 1);                        // 偏移地址
                    uint8_t*  type   = (uint8_t*) (offset + 1);                      // 数据类型
                    uint8_t*  data   = NULL;                                         // 数据指针
                    uint8_t*  chksum = NULL;                                         // 校验和

                    *head = '\0';   // 头部
                    /* 将字符串的数据转换为数值 */
                    for (uint16_t i = 1; i < (end - poi); i += 2) {
                        for (uint16_t j = 0; j < 2; j++) {
                            buf[i / 2 + 1] <<= 4;
                            if (poi[i + j] >= '0' && poi[i + j] <= '9') {
                                buf[i / 2 + 1] |= (poi[i + j] - '0');
                            } else if (poi[i + j] >= 'A' && poi[i + j] <= 'F') {
                                buf[i / 2 + 1] |= (poi[i + j] - 'A' + 10);
                            } else if (poi[i + j] >= 'a' && poi[i + j] <= 'f') {
                                buf[i / 2 + 1] |= (poi[i + j] - 'a' + 10);
                            } else {
                                continue;
                            }
                        }
                    }
                    if (*len != 0) {            // 有数据
                        data   = type + 1;      // 更新数据指针
                        chksum = data + *len;   // 更新校验和指针
                    } else {                    // 无数据
                        chksum = type + 1;      // 更新校验和指针
                    }
                    /* 计算数据的校验和 */
                    uint8_t sum = 0;
                    for (uint16_t i = 0; i < (*len) + 4; i++) {
                        sum += buf[i + 1];
                    }
                    sum = 0x100 - sum;
                    /* 检查校验和 */
                    if (sum != *chksum) {
                        break;
                    }
                    switch (*type) {
                        /* 数据记录 */
                        case 0x00: {
                            *offset = (*offset << 8) | (*offset >> 8);   // 数据记录
                            w_addr  = ext_addr + seg_addr + *offset;     // 写入地址
                            if (start_addr == 0xFFFFFFFF) {              // 如果起始地址未设置
                                start_addr = w_addr;                     // 计算起始地址
                            }
                            w_addr -= start_addr;                  // 减去起始地址
                            w_addr += SPI_FLASH_PROGRAM_ADDRESS;   // 烧录地址
                            if (prog_size < w_addr + *len) {
                                prog_size = w_addr + *len;   // 更新烧录大小
                            }
                            {
                                uint32_t w_cnt = 0;
                                while (*len > w_cnt) {
                                    if ((w_addr >= buf_addr) && (w_addr < buf_addr + W25QXX_BLOCK_SIZE)) {
                                        uint32_t offset = w_addr - buf_addr;
                                        if (*len > W25QXX_BLOCK_SIZE - offset) {
                                            memcpy(&w_buf[offset], &data[w_cnt], W25QXX_BLOCK_SIZE - offset);
                                            w_cnt += W25QXX_BLOCK_SIZE - offset;
                                            w_addr += W25QXX_BLOCK_SIZE - offset;   // 更新写入地址
                                        } else {
                                            memcpy(&w_buf[offset], &data[w_cnt], *len);
                                            w_cnt += *len;
                                            w_addr += *len;   // 更新写入地址
                                        }
                                    } else {
                                        FLASH_BUF_SYNC();
                                        LED_OnOff(ERR);
                                        buf_addr = w_addr & ~(W25QXX_BLOCK_SIZE - 1);
                                        if (FLASH_W_FLAG(buf_addr) != 1) {
                                            memset(w_buf, 0xFF, W25QXX_BLOCK_SIZE);
                                        } else {
                                            W25QXX_Read(w_buf, buf_addr, W25QXX_BLOCK_SIZE);
                                        }
                                    }
                                }
                            }
                            LED_OnOff(RUN);
                        } break;
                        /* 结束记录 */
                        case 0x01: {
                            end_flag = 1;
                        } break;
                        /* 扩展段地址记录 */
                        case 0x02: {
                            seg_addr = ((data[0] << 8) | data[1]) << 16;   // 段地址
                        } break;
                        /* 起始段地址记录 */
                        case 0x03: {
                        } break;
                        /* 扩展线性地址记录 */
                        case 0x04: {
                            ext_addr = (data[0] << 8 | data[1]) << 16;
                        } break;
                        /* 线性地址记录 */
                        case 0x05: {
                        } break;
                    }
                    poi   = end + 1;   // 移动处理指针到下一行
                    r_poi = poi;       // 更新读取指针
                }
            }
            FLASH_BUF_SYNC();   // 同步缓冲区
            vPortFree(w_buf);   // 释放写入缓冲区
            for (uint32_t i = SPI_FLASH_PROGRAM_ADDRESS;
                 i < prog_size;
                 i += W25QXX_BLOCK_SIZE) {
                if (FLASH_W_FLAG(i) == 0) {
                    FLASH_W_FLAG(i) = 1;
                    W25QXX_EraseSector(i);
                }
            }
            prog_size -= SPI_FLASH_PROGRAM_ADDRESS;       // 减去烧录地址
            burner_addr_update            = 1;            // 烧录地址更新标志
            BurnerConfigInfo.FlashAddress = start_addr;   // 更新烧录地址
        }
        LED_Off(RUN);
        LED_Off(ERR);
        f_res = f_close(file);
        /* 删除文件 */
        f_res = f_unlink(BurnerConfigInfo.FilePath);

        BurnerConfigInfo.FileAddress = SPI_FLASH_PROGRAM_ADDRESS;   // 获取文件地址
        BurnerConfigInfo.FileSize    = prog_size;                   // 获取文件大小
    } while (0);

    /********************************* 检查配置文件 *********************************/
    {
        crc      = 0;      // CRC校验码
        *str_buf = '\0';   // 清空字符串缓冲区
        if (f_open(file, Config_Path, FA_READ | FA_WRITE | FA_OPEN_ALWAYS) != FR_OK) {
            goto ex;
        }
        /* 读取 */
        if ((f_size(file) != 0) && (f_size(file) < CONFIG_BUFFER_SIZE)) {
            f_res = f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt);
            crc   = CRC32_Update(0, str_buf, r_cnt);   // 计算CRC32校验码
        }

        cJSON* root = NULL;   // JSON根对象
        cJSON* item = NULL;   // JSON项

        root = cJSON_ParseWithLength(str_buf, r_cnt);   // 解析JSON字符串
        if (root == NULL) {
            root = cJSON_CreateObject();   // 创建一个新的JSON对象
        }
        /* 保存烧录文件名 */
        if ((item = cJSON_GetObjectItem(root, "file")) != NULL) {
            cJSON_SetValuestring(item, BurnerConfigInfo.FilePath);
        } else {
            cJSON_AddStringToObject(root, "file", BurnerConfigInfo.FilePath);
        }
        /* 保存烧录文件大小 */
        if ((item = cJSON_GetObjectItem(root, "fileSize")) != NULL) {
            cJSON_SetNumberValue(item, BurnerConfigInfo.FileSize);
        } else {
            cJSON_AddNumberToObject(root, "fileSize", BurnerConfigInfo.FileSize);
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
        // CONFIG_OBJECT_INT(root, "chipLock", uint8_t, BurnerConfigInfo.ChipLock, CONFIG_DEFAULT_CHIP_LOCK);
        CONFIG_OBJECT_INT(root, "autoRun", uint8_t, BurnerConfigInfo.AutoRun, CONFIG_DEFAULT_AUTO_RUN);
        /* 烧录地址 */
        if ((item = cJSON_GetObjectItem(root, "flashAddr")) != NULL) {
            if (cJSON_IsString(item)) {
                if (burner_addr_update == 1) {
                    char flash_addr[16];
                    sprintf(flash_addr, "0x%08X", BurnerConfigInfo.FlashAddress);
                    cJSON_SetValuestring(item, flash_addr);
                } else {
                    char* flash_addr = cJSON_GetStringValue(item);
                    if (HEX2DEC(flash_addr, &BurnerConfigInfo.FlashAddress) == 0) {
                        cJSON_SetValuestring(item, CONFIG_DEFAULT_FLASH_ADDRESS);
                        HEX2DEC(CONFIG_DEFAULT_FLASH_ADDRESS, &BurnerConfigInfo.FlashAddress);
                    }
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
            f_res = f_write(file, json, strlen(json), &r_cnt);
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
    if (f_open(file, Readme_Path, FA_WRITE | FA_READ | FA_OPEN_ALWAYS) != FR_OK) {
        goto ex;
    }
    if (f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt) != FR_OK) {
        goto ex;
    }
    crc = CRC32_Update(0, str_buf, r_cnt);   // 计算CRC32校验码
    if (crc != CRC32_Update(0, (void*) ConfigReadme, strlen(ConfigReadme))) {
        f_res = f_lseek(file, 0);
        f_res = f_write(file, ConfigReadme, strlen(ConfigReadme), &r_cnt);
        f_res = f_truncate(file);
    }
    f_close(file);

    /********************************* 检查支持列表文件 *********************************/
    if (f_open(file, Supported_Path, FA_WRITE | FA_READ | FA_OPEN_ALWAYS) != FR_OK) {
        goto ex;
    }
    if (f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt) != FR_OK) {
        goto ex;
    }
    crc = CRC32_Update(0, str_buf, r_cnt);   // 计算CRC32校验码
    FlashBlob_ListStr(str_buf);              // 获取支持列表字符串
    if (crc != CRC32_Update(0, (void*) str_buf, strlen(str_buf))) {
        f_res = f_lseek(file, 0);
        f_res = f_write(file, str_buf, strlen(str_buf), &r_cnt);
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
