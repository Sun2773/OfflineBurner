#include "string.h"

/**
 * @brief  判断字符串开头
 * @note   判断字符串src是否以字符串head为开头
 * @param  src: 被检查字符串
 * @param  head: 检查用字符串
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
 * @brief  在str1中寻找第一次出现str2的位置，最多查找n字节
 * @note
 * @param  str1: 被查找字符串
 * @param  str2: 查找字符串
 * @param  n: 最大查找大小
 * @retval 返回查找到的位置
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