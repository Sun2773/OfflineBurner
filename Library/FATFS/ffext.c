#include "ffext.h"
#include "stdio.h"
#include "string.h"

/**
 * @brief  �����ļ�/�ļ���
 * @note   �������·�����ļ��У���ݹ鸴���ļ����ڵ��ļ�
 * @param  path_old: Դ�ļ�·��
 * @param  path_new: Ŀ���ļ�·��
 * @retval ִ�н��
 */
FRESULT f_cpname(const TCHAR* path_old, const TCHAR* path_new) {
    FRESULT  f_res        = FR_OK;                          // �ļ�ϵͳ�������
    FILINFO* f_info       = ff_memalloc(sizeof(FILINFO));   // �ļ���Ϣ����
    DIR      dir          = {0};                            // �ļ��ж���
    BYTE*    buffer       = NULL;                           // �ļ�����
    FIL*     file_old     = NULL;                           // ԭʼ�ļ�����
    FIL*     file_new     = NULL;                           // ��Ŀ���ļ�����
    TCHAR*   path_sub_old = NULL;                           // ԭʼ��·��
    TCHAR*   path_sub_new = NULL;                           // ��Ŀ����·��
    UINT     rw_cnt       = 0;                              // ��д����

    /* ���·�� */
    if ((f_res = f_stat(path_old, f_info)) != FR_OK) {
        goto ex;
    }
    /* ��·���Ǹ��ļ��� */
    if (f_info->fattrib & AM_DIR) {
        if ((f_res = f_mkdir(path_new)) != FR_OK && f_res != FR_EXIST) {
            goto ex;
        }
        if ((f_res = f_opendir(&dir, path_old)) != FR_OK) {
            goto ex;
        }
        while (1) {
            /* ��ȡ�ļ� */
            if ((f_res = f_readdir(&dir, f_info)) != FR_OK) {
                goto ex;
            }
            /* ���ļ����� */
            if (*f_info->fname == NULL) {
                break;
            }
            /* ������·�� */
            path_sub_old = ff_memalloc((strlen(path_old) + strlen(f_info->fname) + 2) +
                                       (strlen(path_new) + strlen(f_info->fname) + 2));
            path_sub_new = path_sub_old + (strlen(path_old) + strlen(f_info->fname) + 2);
            sprintf(path_sub_old, "%s/%s", path_old, f_info->fname);
            sprintf(path_sub_new, "%s/%s", path_new, f_info->fname);
            /* �ݹ鸴�� */
            f_res = f_cpname(path_sub_old, path_sub_new);
            /* �ͷŻ��� */
            ff_memfree(path_sub_old);
        }
        f_closedir(&dir);
    }
    /* ��·�����ļ� */
    else if (f_info->fattrib & AM_ARC) {
        file_old = (FIL*) ff_memalloc(sizeof(FIL) * 2 + FF_MAX_SS);
        file_new = (FIL*) &file_old[1];
        buffer   = (BYTE*) &file_new[1];
        /* ֻ����Դ�ļ� */
        if ((f_res = f_open(file_old, path_old, FA_READ)) != FR_OK) {
            goto ex;
        }
        /* ��λ�ļ�ָ�� */
        if ((f_res = f_lseek(file_old, 0)) != FR_OK) {
            goto ex;
        }
        /* ��Ŀ���ļ� */
        if ((f_res = f_open(file_new, path_new, FA_WRITE | FA_CREATE_NEW)) != FR_OK) {
            goto ex;
        }
        /* ��λ�ļ�ָ�� */
        if ((f_res = f_lseek(file_new, 0)) != FR_OK) {
            goto ex;
        }
        /* ��ʼ�����ļ� */
        while (1) {
            /* ��ȡ�ļ� */
            if ((f_res = f_read(file_old, buffer, FF_MAX_SS, &rw_cnt)) != FR_OK) {
                goto ex;
            }
            /* ��ȡ��� */
            if (rw_cnt == 0) {
                break;
            }
            /* ��ȡ�ļ� */
            if ((f_res = f_write(file_new, buffer, rw_cnt, &rw_cnt)) != FR_OK) {
                goto ex;
            }
        }
        /* ������ɣ��ر��ļ� */
        f_res = f_close(file_old);
        f_res = f_close(file_new);
    }
ex:
    /* �ͷ��ڴ� */
    if (f_info != NULL) {
        ff_memfree(f_info);
    }
    if (file_old != NULL) {
        ff_memfree(file_old);
    }
    /* ���ؽ�� */
    return f_res;
}

/**
 * @brief  ɾ���ļ�/�ļ���
 * @note   �������·�����ļ��У���ݹ�ɾ���ļ����ڵ��ļ�
 * @param  path: Ŀ��·��
 * @retval ִ�н��
 */
FRESULT f_del(const TCHAR* path) {
    FRESULT  f_res    = FR_OK;                          // �ļ�ϵͳ�������
    FILINFO* f_info   = ff_memalloc(sizeof(FILINFO));   // �ļ���Ϣ����
    DIR      dir      = {0};                            // �ļ��ж���
    TCHAR*   path_sub = NULL;                           // ԭʼ��·��

    /* ���·�� */
    if ((f_res = f_stat(path, f_info)) != FR_OK) {
        goto ex;
    }
    /* ��·���Ǹ��ļ��� */
    if (f_info->fattrib & AM_DIR) {
        if ((f_res = f_opendir(&dir, path)) != FR_OK) {
            goto ex;
        }
        while (1) {
            /* ��ȡ�ļ� */
            if ((f_res = f_readdir(&dir, f_info)) != FR_OK) {
                goto ex;
            }
            /* ���ļ����� */
            if (*f_info->fname == NULL) {
                break;
            }
            /* ������·�� */
            path_sub = ff_memalloc((strlen(path) + strlen(f_info->fname) + 2));
            sprintf(path_sub, "%s/%s", path, f_info->fname);
            /* �ݹ鸴�� */
            f_res = f_del(path_sub);
            /* �ͷŻ��� */
            ff_memfree(path_sub);
        }
        f_closedir(&dir);
        f_res = f_unlink(path);
    }
    /* ��·�����ļ� */
    else if (f_info->fattrib & AM_ARC) {
        f_res = f_unlink(path);
    }
ex:
    /* �ͷ��ڴ� */
    if (f_info != NULL) {
        ff_memfree(f_info);
    }
    /* ���ؽ�� */
    return f_res;
}
