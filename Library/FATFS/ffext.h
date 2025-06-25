#ifndef __FFEXT_H__
#define __FFEXT_H__

#include "ff.h"

FRESULT f_cpname(const TCHAR* path_old, const TCHAR* path_new); /* 复制文件/文件夹 */
FRESULT f_del(const TCHAR* path);                               /* 删除文件/文件夹 */

#endif