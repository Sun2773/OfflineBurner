#ifndef __TASK_BURNER_H__
#define __TASK_BURNER_H__

#include "flash_blob.h"
#include "stdlib.h"
#include "stm32f10x.h"

#define BURNER_AUTO_START_TIME (1000 / 100)   // 识别后启动烧录时间
#define BURNER_AUTO_END_TIME   (500 / 100)    // 断开后结束烧录时间

#define BURNER_RETRY_COUNT 2   // 烧录失败重试次数

typedef enum {
    BURNER_ERROR_NONE = 0,        // 无错误
    BURNER_ERROR_INIT,            // 初始化失败
    BURNER_ERROR_BUFFER,          // 缓存分配失败
    BURNER_ERROR_OPT_INIT,        // 选项字初始化失败
    BURNER_ERROR_OPT_ERASE,       // 选项字擦除失败
    BURNER_ERROR_OTP_SETRDP,      // 选项字设置RDP失败
    BURNER_ERROR_FLASH_INIT,      // Flash初始化失败
    BURNER_ERROR_FLASH_ALGO,      // Flash编程算法错误
    BURNER_ERROR_FLASH_ERASE,     // Flash擦除失败
    BURNER_ERROR_FLASH_PROGRAM,   // Flash编程失败
    BURNER_ERROR_FLASH_VERIFY,    // Flash校验失败
    BURNER_ERROR_CHIP_UNKNOWN,    // 未知芯片
    BURNER_ERROR_READ_FAIL,       // 读取失败
    BURNER_ERROR_FLASH_SIZE,      // 读取Flash大小失败
} Burner_Error_t;

typedef enum {
    BURNER_STATE_IDLE = 0,   // 空闲状态
    BURNER_STATE_START,      // 启动状态
    BURNER_STATE_READY,      // 准备状态
    BURNER_STATE_RUNNING,    // 运行状态
    BURNER_STATE_FINISH,     // 完成状态
    BURNER_STATE_LOCK,       // 锁定状态
} Burner_State_t;

typedef struct {
    uint8_t          Online;                          // 在线状态
    uint8_t          ErrCnt;                          // 错误计数
    int16_t          StartTimer;                      // 启动计时器
    int16_t          EndTimer;                        // 结束计时器
    Burner_State_t   State;                           // 工作状态
    Burner_Error_t   Error;                           // 错误码
    Burner_Error_t   ErrorList[BURNER_RETRY_COUNT];   // 错误码
    uint8_t*         Buffer;                          // 烧录数据缓冲区
    FlashBlobList_t* FlashBlob;                       // 当前Flash编程算法

    struct {
        uint32_t ChipIdcode;   // 芯片ID
        union {
            uint32_t DBGMCU_IDCODE;   // 调试MCU IDCODE寄存器
            struct {
                uint16_t DEV_ID : 12;   // 设备ID
                uint16_t        : 4;    // 保留位
                uint16_t REV_ID : 16;   // 版本ID
            };
        };
        uint16_t FlashSize;     // Flash大小(Kb)
        uint32_t ProgramSize;   // 程序大小
        uint32_t FinishSize;    // 已完成大小
        uint16_t FinishRate;    // 完成率
        uint32_t FinishTime;    // 完成时间
    } Info;
} BurnerCtrl_t;

extern BurnerCtrl_t BurnerCtrl;

void Burner_Task(void);
void Burner_Detection(void);
void Burner_Exe(void);

#endif   // __TASK_BURNER_H__