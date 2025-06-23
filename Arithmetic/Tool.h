#ifndef __TOOL_H__
#define __TOOL_H__

#include "stdlib.h"

/**
 * @brief  ð����������
 * @note   ��С����
 * @retval NULL
 */
#define BubbleSort_P(c, a)                                    \
    {                                                         \
        for (uint16_t i = 0; i < ArraySize(a) - 1; i++) {     \
            for (uint16_t j = i + 1; j < ArraySize(a); j++) { \
                if (a[i] > a[j]) {                            \
                    c t  = a[i];                              \
                    a[i] = a[j];                              \
                    a[j] = t;                                 \
                }                                             \
            }                                                 \
        }                                                     \
    }

/**
 * @brief  ð��������
 * @note   �ɴ���С
 * @retval NULL
 */
#define BubbleSort_N(c, a)                                    \
    {                                                         \
        for (uint16_t i = 0; i < ArraySize(a) - 1; i++) {     \
            for (uint16_t j = i + 1; j < ArraySize(a); j++) { \
                if (a[i] < a[j]) {                            \
                    c t  = a[i];                              \
                    a[i] = a[j];                              \
                    a[j] = t;                                 \
                }                                             \
            }                                                 \
        }                                                     \
    }

/**
 * @brief  �������в���һ��
 * @note   ���뵽����ͷ��
 */
#define List_Insert(class, head, item)   \
    {                                    \
        if ((head) == NULL) {            \
            (item)->Prev = (item);       \
            (item)->Next = NULL;         \
        } else {                         \
            (item)->Next = (head);       \
            (item)->Prev = (head)->Prev; \
            (head)->Prev = (item);       \
        }                                \
        (head) = (item);                 \
    }

/**
 * @brief  �������в���һ��
 * @note   ���뵽����β��
 */
#define List_InsertEnd(class, head, item)  \
    {                                      \
        if ((head) == NULL) {              \
            item->Prev = (item);           \
            (head)     = (item);           \
        } else {                           \
            class* _index = (head);        \
            while (_index->Next != NULL) { \
                _index = _index->Next;     \
            }                              \
            _index->Next = (item);         \
            (item)->Prev = _index;         \
            (head)->Prev = (item);         \
        }                                  \
        (item)->Next = NULL;               \
    }

/**
 * @brief  ���������Ƴ�һ��
 * @note
 */
#define List_Remove(class, head, item)     \
    {                                      \
        if (item->Prev->Next != NULL) {    \
            item->Prev->Next = item->Next; \
        }                                  \
        if (item->Next != NULL) {          \
            item->Next->Prev = item->Prev; \
        }                                  \
        if (head == item) {                \
            head = item->Next;             \
        }                                  \
    }

/* �ж��ַ�����ͷ */
int StrStartWith(char* src, char* head);
/* ��str1��Ѱ�ҵ�һ�γ���str2��λ�ã�������n�ֽ� */
char* strnstr(char* str1, char* str2, size_t n);

#define ArraySize(array) (sizeof(array) / sizeof(array[0]))

#define BIT_VAL(addr, bitnum) *((uint32_t*) ((((uint32_t) addr & 0xF0000000) + 0x2000000 + (((uint32_t) addr & 0xFFFFF) << 5) + (bitnum << 2))))

#define IF_BIT(reg, bit)    if ((reg & bit) == bit)
#define IF_NO_BIT(reg, bit) if ((reg & bit) != bit)

#define SCOPE(v, min, max) (((v) <= (min)) ? (min) : (((v) >= (max)) ? (max) : (v)))
#define SCOPE_MIN(v, min)  (((v) <= (min)) ? (min) : (v))
#define SCOPE_MAX(v, max)  (((v) >= (max)) ? (max) : (v))

#endif
