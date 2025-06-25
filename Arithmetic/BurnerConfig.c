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

struct {
    uint32_t Flag;            // Flash��С
    uint32_t FileAddress;     // �ļ���ַ
    uint32_t FileSize;        // �ļ���С
    uint32_t FileCrc;         // �ļ�CRC32У����
    char     FilePath[128];   // �ļ�·��
} BurnerConfigInfo = {
    .FilePath = "",
};

void BurnerConfig(void) {
    FATFS*   fs          = NULL;    // �ļ�ϵͳ����
    FIL*     file        = NULL;    // �ļ�����
    FILINFO* file_info   = NULL;    // �ļ���Ϣ����
    FRESULT  f_res       = FR_OK;   // FATFS�������
    char*    str_buf     = NULL;    // �ַ���������
    UINT     r_cnt       = 0;       // ��ȡ���
    uint32_t config_flag = 0;       // ���ñ�־
    /* ��ȡFlash���� */
    SPI_FLASH_Read(&BurnerConfigInfo,
                   FLASH_CONFIG_ADDRESS,
                   sizeof(BurnerConfigInfo));
    config_flag = BurnerConfigInfo.Flag;   // ��ȡ���ñ�־
    /* ������ñ�־Ϊ0xFFFFFFFF����ʾû������ */
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
    /* �����ļ���ͳ */
    f_res = f_mount(fs, Flash_Path, 1);
    /* û���ļ�ϵͳ */
    if (f_res == FR_NO_FILESYSTEM) {
        /* ��ʽ��Flash */
        f_res = f_mkfs("0:", 0, NULL, 4096);
        /* ȡ������ */
        f_res = f_mount(0, "0:", 1);
        /* �ٴι��� */
        f_res = f_mount(fs, "0:", 1);
    }
    /* ����ʧ�� */
    if (f_res != FR_OK) {
        goto ex;
    }

    do {
        DIR f_dp = {0};   // Ŀ¼����
        /* ��Ŀ¼ */
        if ((f_res = f_opendir(&f_dp, Flash_Path)) != FR_OK) {
            goto ex;
        }
        /* Ѱ�ҹ̼��ļ� */
        while (1) {
            /* ��ȡ�ļ� */
            f_readdir(&f_dp, file_info);
            /* ���ļ����� */
            if (*file_info->fname == '\0') {
                break;
            }
            /* �ų��ļ��� */
            if ((file_info->fattrib & AM_DIR) != 0) {
                continue;
            }
            /* �ж��Ƿ�Ϊ�̼��ļ� */
            if (((memcmp(strrchr(file_info->fname, '.'), ".bin", 4) == 0) ||
                 (memcmp(strrchr(file_info->fname, '.'), ".BIN", 4) == 0))) {
                break;
            }
        }

        /* ���û���ҵ��̼��ļ� */
        if (*file_info->fname == '\0') {
            break;
        }
        *BurnerConfigInfo.FilePath = '\0';                     // ����ļ�·��
        strcat(BurnerConfigInfo.FilePath, Flash_Path);         // ����Flash·��
        strcat(BurnerConfigInfo.FilePath, file_info->fname);   // ׷���ļ���
        /* �򿪹̼��ļ� */
        if ((f_res = f_open(file, BurnerConfigInfo.FilePath, FA_READ)) != FR_OK) {
            break;
        }
        uint32_t file_size   = f_size(file);   // �ļ���С
        uint32_t file_finish = 0;              // ����ɴ�С
        uint32_t rw_addr     = 0;              // ��д��ַ
        uint32_t data_crc32  = 0;              // ����У����
        uint32_t file_crc32  = 0;              // �ļ�У����

        /* ��ʼ�����ļ� */
        while (file_size - file_finish) {
            if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
                /* ���ʣ���ֽ���,��ʣ���ֽڴ��ڻ���,��ȡ�����С�ļ� */
                r_cnt = CONFIG_BUFFER_SIZE;
            } else {
                /* ʣ���ֽ�������0С�ڻ���,��ȡʣ���ֽ��� */
                r_cnt = (file_size - file_finish);
            }
            /* ��ȡ�ļ� */
            if (f_read(file, str_buf, r_cnt, &r_cnt) != FR_OK) {
                break;
            }

            /* �����ļ�У���� */
            file_crc32 = CRC32_Update(file_crc32, str_buf, r_cnt);
            rw_addr    = FLASH_PROGRAM_ADDRESS + file_finish;   // д���ַ
            if ((rw_addr % W25QXX_BLOCK_SIZE) == 0) {
                /* ���д���ַ�ǿ��С���������������� */
                SPI_FLASH_Erase(rw_addr);
            }
            SPI_FLASH_Write(str_buf, rw_addr, r_cnt);
            /* ���� */
            file_finish += r_cnt;
            LED_OnOff(RUN);
        }
        /* ���¸�ֵ */
        file_size   = file_finish;
        file_finish = 0;
        /* ��ʼУ���ļ� */
        while (file_size - file_finish) {
            /* ���ʣ���ֽ���,��ʣ���ֽڴ��ڻ��� */
            if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
                /* ��ȡ�����С�ļ� */
                r_cnt = CONFIG_BUFFER_SIZE;
            }
            /* ʣ���ֽ�������0С�ڻ��� */
            else {
                /* ��ȡʣ���ֽ��� */
                r_cnt = (file_size - file_finish);
            }
            rw_addr = FLASH_PROGRAM_ADDRESS + file_finish;   // ��ȡ��ַ
            /* ��ȡ���� */
            SPI_FLASH_Read(str_buf, rw_addr, r_cnt);
            /* ��������У���� */
            data_crc32 = CRC32_Update(data_crc32, str_buf, r_cnt);
            /* ���� */
            file_finish += r_cnt;
            LED_OnOff(RUN);
        }
        /* У������ */
        if (data_crc32 == file_crc32) {
            BurnerConfigInfo.FileAddress = FLASH_PROGRAM_ADDRESS;   // ��ȡ�ļ���ַ
            BurnerConfigInfo.FileSize    = file_finish;             // ��ȡ�ļ���С
            BurnerConfigInfo.FileCrc     = file_crc32;              // �����ļ�CRC32У����
            BurnerConfigInfo.Flag        = 0;                       // �������ñ�־
            /* ɾ���ļ� */
            f_res = f_unlink(BurnerConfigInfo.FilePath);
        } else {
        }
    } while (0);

    do {
        if ((f_res = f_stat(Config_Path, file_info)) == FR_OK) {
            config_flag = ((uint32_t) (file_info->fdate) << 16) | file_info->ftime;
        }
        /* �����ļ������仯����ȡ�����ļ� */
        if (config_flag != BurnerConfigInfo.Flag || f_res == FR_NO_FILE) {
            if ((f_res = f_open(file, Config_Path, FA_READ | FA_WRITE | FA_OPEN_ALWAYS)) != FR_OK) {
                goto ex;
            }
            // ����ļ������ڣ�����һ��Ĭ������
            uint32_t file_size = f_size(file);

            if ((file_size != 0) && (file_size < CONFIG_BUFFER_SIZE)) {
                f_res = f_read(file, str_buf, CONFIG_BUFFER_SIZE, &r_cnt);
            }

            cJSON* root = NULL;   // JSON������
            cJSON* item = NULL;   // JSON��

            root = cJSON_ParseWithLength(str_buf, r_cnt);   // ����JSON�ַ���
            if (root == NULL) {
                root = cJSON_CreateObject();   // ����һ���µ�JSON����
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
            str_buf = cJSON_PrintUnformatted(root);   // ��JSON����ת��Ϊ�ַ���
            cJSON_Delete(root);                       // ɾ��JSON����

            f_res = f_lseek(file, 0);   // ���ļ�ָ���ƶ����ļ���ͷ
            f_res = f_write(file, str_buf, strlen(str_buf), &r_cnt);
            f_res = f_truncate(file);   // �ض��ļ�����ǰ��С

            f_close(file);

            f_res       = f_stat(Config_Path, file_info);
            config_flag = ((uint32_t) (file_info->fdate) << 16) | file_info->ftime;
        }
    } while (0);
    if (config_flag != BurnerConfigInfo.Flag) {
        BurnerConfigInfo.Flag = config_flag;
        SPI_FLASH_Erase(FLASH_CONFIG_ADDRESS);   // ����Flash���õ�ַ
        SPI_FLASH_Write(&BurnerConfigInfo,
                        FLASH_CONFIG_ADDRESS,
                        sizeof(BurnerConfigInfo));   // д��Flash����
    }
ex:
    if (file != NULL) {
        f_close(file);
        vPortFree(file);
    }
    /*ȡ������*/
    f_res = f_mount(0, "0:", 1);

    if (fs != NULL) {
        vPortFree(fs);
    }
    if (str_buf != NULL) {
        vPortFree(str_buf);
    }
    return;
}