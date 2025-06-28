#ifndef __FLASH_BLOB_H__
#define __FLASH_BLOB_H__

#include <stdint.h>

typedef struct {
    uint32_t breakpoint;      // 断点地址 (算法起始+1，Thumb模式)
    uint32_t static_base;     // 静态数据基址
    uint32_t stack_pointer;   // 栈指针地址
} program_syscall_t;

typedef struct {
    unsigned long szSector;     // 扇区大小（以字节为单位）
    unsigned long AddrSector;   // 扇区地址
} sector_info_t;

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
    const sector_info_t*    sector_info;           // 扇区信息
    const uint32_t          sector_info_count;     // 扇区数量
} program_target_t;

typedef struct {
    const uint16_t DevId;           // 设备ID (12位)
    const char*    Name;            // 设备名称
    const uint32_t FlashSizeAddr;   // Flash大小寄存器地址
    const uint16_t FlashSize[2];    // Flash大小范围

    const program_target_t* prog_flash;   // Flash编程算法
    const program_target_t* prog_opt;     // 选项字编程算法
} FlashBlobList_t;

FlashBlobList_t* FlashBlob_Get(uint16_t id, uint16_t flash_size);

#endif
