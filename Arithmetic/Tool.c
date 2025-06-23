#include "string.h"

/**
 * @brief  �ж��ַ�����ͷ
 * @note   �ж��ַ���src�Ƿ����ַ���headΪ��ͷ
 * @param  src: ������ַ���
 * @param  head: ������ַ���
 * @retval
 */
int StrStartWith(char* src, char* head) {
    while (*head != '\0') {
        if (*src++ != *head++) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief  ��str1��Ѱ�ҵ�һ�γ���str2��λ�ã�������n�ֽ�
 * @note
 * @param  str1: �������ַ���
 * @param  str2: �����ַ���
 * @param  n: �����Ҵ�С
 * @retval ���ز��ҵ���λ��
 */
char* strnstr(char* str1, char* str2, size_t n) {
    size_t l1, l2;
    if ((l2 = strlen(str2)) == 0)
        return NULL;
    if ((l1 = strlen(str1)) < n)
        n = l1;
    while (n-- >= l2) {
        if (memcmp(str1, str2, l2) == 0)
            return str1;
        str1++;
    }
    return NULL;
}