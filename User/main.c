
#include "stm32f10x.h"

#include "stdio.h"
#include "Tool.h"

/***************** 类型声明 *****************/

typedef struct {
    void (*Hook)(void);   // 任务钩子函数
    uint16_t Cycle;       // 任务周期
    uint16_t Timer;       // 计数器
    uint8_t  Ready;       // 任务就绪状态
} TaskUnti_t;             // 任务单元定义

/***************** 变量声明 *****************/

RCC_ClocksTypeDef RCC_Clocks;   // 系统时钟频率

/***************** 函数声明 *****************/

void Task_Process(void);   // 任务处理函数
void TaskNull(void);       // 空任务

/***************** 任务定义 *****************/

TaskUnti_t TaskList[] = {
    /* 任务钩子，执行周期 */
    {TaskNull, 1000},

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

    /* 外设初始化 */

    /* 进行任务处理 */
    Task_Process();
    while (1) {
    }
}

/***************** 任务调度功能 *****************/

/**
 * @brief  任务调度函数
 * @note
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
}

/**
 * @brief  任务处理函数
 * @note
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
