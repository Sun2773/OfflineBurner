#ifndef __FFEXT_H__
#define __FFEXT_H__

#include "ff.h"

FRESULT f_cpname(const TCHAR* path_old, const TCHAR* path_new); /* �����ļ�/�ļ��� */
FRESULT f_del(const TCHAR* path);                               /* ɾ���ļ�/�ļ��� */

#endif