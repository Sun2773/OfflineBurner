#ifndef FLASH_BLOB_H
#define FLASH_BLOB_H

#include <stdint.h>

typedef struct {
    uint32_t breakpoint;      // 断点地址 (算法起始+1，Thumb模式)
    uint32_t static_base;     // 静态数据基址
    uint32_t stack_pointer;   // 栈指针地址
} program_syscall_t;

typedef struct {
    const uint32_t          init;                  // 初始化函数地址
    const uint32_t          uninit;                // 反初始化函数地址
    const uint32_t          erase_chip;            // 全片擦除函数地址
    const uint32_t          erase_sector;          // 扇区擦除函数地址
    const uint32_t          program_page;          // 扇区擦除函数地址
    const program_syscall_t sys_call_s;            // 系统调用参数
    const uint32_t          program_buffer;        // 编程缓冲区地址
    const uint32_t          algo_start;            // 算法代码起始地址
    const uint32_t          algo_size;             // 算法代码大小
    const uint32_t*         algo_blob;             // 算法代码数据指针
    const uint32_t          program_buffer_size;   // 编程缓冲区大小
} program_target_t;

typedef struct {
    const uint32_t start;
    const uint32_t size;
} sector_info_t;

#endif
