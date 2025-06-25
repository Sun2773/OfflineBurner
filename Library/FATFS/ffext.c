#include "ffext.h"
#include "stdio.h"
#include "string.h"

/**
 * @brief  复制文件/文件夹
 * @note   若传入的路径是文件夹，则递归复制文件夹内的文件
 * @param  path_old: 源文件路径
 * @param  path_new: 目标文件路径
 * @retval 执行结果
 */
FRESULT f_cpname(const TCHAR* path_old, const TCHAR* path_new) {
    FRESULT  f_res        = FR_OK;                          // 文件系统操作结果
    FILINFO* f_info       = ff_memalloc(sizeof(FILINFO));   // 文件信息对象
    DIR      dir          = {0};                            // 文件夹对象
    BYTE*    buffer       = NULL;                           // 文件缓存
    FIL*     file_old     = NULL;                           // 原始文件对象
    FIL*     file_new     = NULL;                           // 新目标文件对象
    TCHAR*   path_sub_old = NULL;                           // 原始子路径
    TCHAR*   path_sub_new = NULL;                           // 新目标子路径
    UINT     rw_cnt       = 0;                              // 读写计数

    /* 检查路径 */
    if ((f_res = f_stat(path_old, f_info)) != FR_OK) {
        goto ex;
    }
    /* 该路径是个文件夹 */
    if (f_info->fattrib & AM_DIR) {
        if ((f_res = f_mkdir(path_new)) != FR_OK && f_res != FR_EXIST) {
            goto ex;
        }
        if ((f_res = f_opendir(&dir, path_old)) != FR_OK) {
            goto ex;
        }
        while (1) {
            /* 读取文件 */
            if ((f_res = f_readdir(&dir, f_info)) != FR_OK) {
                goto ex;
            }
            /* 无文件结束 */
            if (*f_info->fname == NULL) {
                break;
            }
            /* 生成子路径 */
            path_sub_old = ff_memalloc((strlen(path_old) + strlen(f_info->fname) + 2) +
                                       (strlen(path_new) + strlen(f_info->fname) + 2));
            path_sub_new = path_sub_old + (strlen(path_old) + strlen(f_info->fname) + 2);
            sprintf(path_sub_old, "%s/%s", path_old, f_info->fname);
            sprintf(path_sub_new, "%s/%s", path_new, f_info->fname);
            /* 递归复制 */
            f_res = f_cpname(path_sub_old, path_sub_new);
            /* 释放缓存 */
            ff_memfree(path_sub_old);
        }
        f_closedir(&dir);
    }
    /* 该路径是文件 */
    else if (f_info->fattrib & AM_ARC) {
        file_old = (FIL*) ff_memalloc(sizeof(FIL) * 2 + FF_MAX_SS);
        file_new = (FIL*) &file_old[1];
        buffer   = (BYTE*) &file_new[1];
        /* 只读打开源文件 */
        if ((f_res = f_open(file_old, path_old, FA_READ)) != FR_OK) {
            goto ex;
        }
        /* 复位文件指针 */
        if ((f_res = f_lseek(file_old, 0)) != FR_OK) {
            goto ex;
        }
        /* 打开目标文件 */
        if ((f_res = f_open(file_new, path_new, FA_WRITE | FA_CREATE_NEW)) != FR_OK) {
            goto ex;
        }
        /* 复位文件指针 */
        if ((f_res = f_lseek(file_new, 0)) != FR_OK) {
            goto ex;
        }
        /* 开始复制文件 */
        while (1) {
            /* 读取文件 */
            if ((f_res = f_read(file_old, buffer, FF_MAX_SS, &rw_cnt)) != FR_OK) {
                goto ex;
            }
            /* 读取完成 */
            if (rw_cnt == 0) {
                break;
            }
            /* 读取文件 */
            if ((f_res = f_write(file_new, buffer, rw_cnt, &rw_cnt)) != FR_OK) {
                goto ex;
            }
        }
        /* 复制完成，关闭文件 */
        f_res = f_close(file_old);
        f_res = f_close(file_new);
    }
ex:
    /* 释放内存 */
    if (f_info != NULL) {
        ff_memfree(f_info);
    }
    if (file_old != NULL) {
        ff_memfree(file_old);
    }
    /* 返回结果 */
    return f_res;
}

/**
 * @brief  删除文件/文件夹
 * @note   若传入的路径是文件夹，则递归删除文件夹内的文件
 * @param  path: 目标路径
 * @retval 执行结果
 */
FRESULT f_del(const TCHAR* path) {
    FRESULT  f_res    = FR_OK;                          // 文件系统操作结果
    FILINFO* f_info   = ff_memalloc(sizeof(FILINFO));   // 文件信息对象
    DIR      dir      = {0};                            // 文件夹对象
    TCHAR*   path_sub = NULL;                           // 原始子路径

    /* 检查路径 */
    if ((f_res = f_stat(path, f_info)) != FR_OK) {
        goto ex;
    }
    /* 该路径是个文件夹 */
    if (f_info->fattrib & AM_DIR) {
        if ((f_res = f_opendir(&dir, path)) != FR_OK) {
            goto ex;
        }
        while (1) {
            /* 读取文件 */
            if ((f_res = f_readdir(&dir, f_info)) != FR_OK) {
                goto ex;
            }
            /* 无文件结束 */
            if (*f_info->fname == NULL) {
                break;
            }
            /* 生成子路径 */
            path_sub = ff_memalloc((strlen(path) + strlen(f_info->fname) + 2));
            sprintf(path_sub, "%s/%s", path, f_info->fname);
            /* 递归复制 */
            f_res = f_del(path_sub);
            /* 释放缓存 */
            ff_memfree(path_sub);
        }
        f_closedir(&dir);
        f_res = f_unlink(path);
    }
    /* 该路径是文件 */
    else if (f_info->fattrib & AM_ARC) {
        f_res = f_unlink(path);
    }
ex:
    /* 释放内存 */
    if (f_info != NULL) {
        ff_memfree(f_info);
    }
    /* 返回结果 */
    return f_res;
}
