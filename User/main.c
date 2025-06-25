
#include "stm32f10x.h"

#include "Tool.h"
#include "heap.h"
#include "stdio.h"

#include "Key.h"
#include "SPI_Flash.h"
#include "buzzer.h"
#include "cJSON.h"
#include "led.h"

#include "BurnerConfig.h"
#include "Task_Key.h"

#include "hw_config.h"
#include "usb_desc.h"
#include "usb_lib.h"
#include "usb_pwr.h"

#include "DAP.h"
#include "DAP_config.h"
#include "SWD_host.h"

#include "ff.h" /* Declarations of FatFs API */

/***************** 类型声明 *****************/

typedef struct {
    void (*Hook)(void);   // 任务钩子函数
    uint16_t Cycle;       // 任务周期
    uint16_t Timer;       // 计数器
    uint8_t  Ready;       // 任务就绪状态
} TaskUnti_t;             // 任务单元定义

/***************** 变量声明 *****************/

RCC_ClocksTypeDef RCC_Clocks;   // 系统时钟频率
uint32_t          SysTick_Count = 0;
uint8_t           test_out      = 0;
uint8_t           test_in       = 0;

// SWD_TargetInfo TargetInfo;

// 0x1BA01477
// SWD_STA  SWD_Res;
uint32_t FlashSize = 0;
uint32_t Data[5];
uint8_t  SWD_Res = 0;   // SWD操作结果
uint8_t  Buffer[1024];
uint32_t Flash_Page_Size = 1024;
FATFS    Fs;

/***************** 函数声明 *****************/

void Task_Process(void);      // 任务处理函数
void TaskNull(void);          // 空任务
void Delay(uint32_t delay);   // 延时函数

/***************** 任务定义 *****************/

TaskUnti_t TaskList[] = {
    /* 任务钩子，执行周期 */
    {TaskNull, 10},
    {LED_Task, 50},   // LED任务，每50ms执行一次
    {Key_Task, 10},   // 按键任务，每10ms执行一次

    // 在上面添加任务。。。。
};

/***************** 主函数 *****************/

/**
 * @brief  主函数
 * @param  None
 * @retval None
 */
int main(void) {
    RCC_GetClocksFreq(&RCC_Clocks);                       // 获取系统时钟频率
    SystemCoreClockUpdate();                              // 更新系统时钟频率
    SysTick_Config(RCC_Clocks.SYSCLK_Frequency / 1000);   // 配置SysTick定时器，每1ms中断一次

    /* 时钟初始化 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 外设初始化 */
    LED_Init();      // 初始化LED
    Key_Init();      // 初始化按键
    W25QXX_Init();   // 初始化SPI Flash
    Buzzer_Init();   // 初始化蜂鸣器

    /* 初始化JSON */
    cJSON_Hooks hooks = {
        pvPortMalloc,
        vPortFree,
    };
    cJSON_InitHooks(&hooks);

    BurnerConfig();   // 初始化烧录配置

    Set_System();
    Set_USBClock();
    USB_Interrupts_Config();
    USB_Init();

    SWD_Res = swd_init_debug();   // 初始化SWD调试接口
    // 下载编程算法到目标MCU的SRAM，并初始化
    SWD_Res = swd_read_memory(0x1FFFF7E0, (uint8_t*) &FlashSize, 2);
    if (SWD_Res) {
        asm("nop");   // 如果初始化成功，执行空操作
    }

    FRESULT res;

    // SWD_Init();
    //
    // SWD_Res = SWD_Target_Init(&TargetInfo);
    // // SWD_Target_Reset();
    // SWD_Res = SWD_Target_RegisterRead(0x1FFFF7E0, &FlashSize);
    // SWD_Res = SWD_Target_WordRW();
    // SWD_Res = SWD_Target_RegisterRead(0x08000000, &Data[0]);
    // SWD_Res = SWD_Target_HalfWordRW();
    // SWD_Res = SWD_Target_RegisterRead(0x08000000, &Data[1]);
    // SWD_Res = SWD_Target_ByteRW();
    // SWD_Res = SWD_Target_RegisterRead(0x08000000, &Data[2]);

    //----------------------测试代码区---------------------------
    // while (1) {
    //     SWD_Target_RegisterWrite(0x4001100C, 0x00002000);
    //     Delay(1000);
    //     SWD_Target_RegisterWrite(0x4001100C, 0x00000000);
    //     Delay(1000);
    //     // 测试代码
    // }

    /* 进行任务处理 */
    Task_Process();
    while (1) {
    }
}

/***************** 任务调度功能 *****************/

/**
 * @brief  任务调度函数
 * @note   滴答定时器中断
 * @retval None
 */
void Task_Remarks(void) {
    uint16_t task_max = ArraySize(TaskList);   // 任务总数

    for (uint8_t i = 0; i < task_max; i++) {   // 逐个任务时间处理
        if (TaskList[i].Cycle == 0) {          // 如果周期为0
            continue;                          // 跳过该任务
        }
        if (TaskList[i].Timer > 0) {   // 如果计时器大于0
            TaskList[i].Timer--;       // 计时器递减
        }
        if (TaskList[i].Timer == 0) {                // 如果计时器到0
            TaskList[i].Timer = TaskList[i].Cycle;   // 重新设置计时器
            TaskList[i].Ready = 1;                   // 设置任务就绪标志
        }
    }
    SysTick_Count = SysTick_Count + 1;
}

/**
 * @brief  任务处理函数
 * @note   主循环
 * @retval None
 */
void Task_Process(void) {
    uint16_t task_max = ArraySize(TaskList);   // 任务总数

    for (uint16_t i = 0; i < task_max; i++) {   // 初始化任务列表
        TaskList[i].Ready = 0;                  // 清除就绪标志
        TaskList[i].Timer = i + 1;              // 设置计时器初始值
    }

    while (task_max) {
        for (uint16_t i = 0; i < task_max; i++) {   // 遍历任务列表
            if ((TaskList[i].Ready != 0) ||         // 如果任务就绪
                (TaskList[i].Cycle == 0)) {         // 或者周期为0
                TaskList[i].Hook();                 // 执行任务钩子函数
                TaskList[i].Ready = 0;              // 清除就绪标志
            }
        }
    }
}

/**
 * @brief  空任务
 * @note
 * @retval None
 */
void TaskNull(void) {
    return;
}

/***************** 延时功能 *****************/
uint32_t DelayTimer = 0;   // 延时计数器

/**
 * @brief  进行毫秒级延时
 * @note
 * @param  delay: 延时的毫秒数
 * @retval None
 */
void Delay(uint32_t delay) {
    DelayTimer = delay;
    while (DelayTimer) {
    }
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1) {
    }
}
#endif
