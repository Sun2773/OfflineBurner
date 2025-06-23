
#include "stm32f10x.h"

#include "stdio.h"
#include "Tool.h"

/***************** �������� *****************/

typedef struct {
    void (*Hook)(void);   // �����Ӻ���
    uint16_t Cycle;       // ��������
    uint16_t Timer;       // ������
    uint8_t  Ready;       // �������״̬
} TaskUnti_t;             // ����Ԫ����

/***************** �������� *****************/

RCC_ClocksTypeDef RCC_Clocks;   // ϵͳʱ��Ƶ��

/***************** �������� *****************/

void Task_Process(void);   // ��������
void TaskNull(void);       // ������

/***************** ������ *****************/

TaskUnti_t TaskList[] = {
    /* �����ӣ�ִ������ */
    {TaskNull, 1000},

    // ������������񡣡�����
};

/***************** ������ *****************/

/**
 * @brief  ������
 * @param  None
 * @retval None
 */
int main(void) {
    RCC_GetClocksFreq(&RCC_Clocks);                       // ��ȡϵͳʱ��Ƶ��
    SystemCoreClockUpdate();                              // ����ϵͳʱ��Ƶ��
    SysTick_Config(RCC_Clocks.SYSCLK_Frequency / 1000);   // ����SysTick��ʱ����ÿ1ms�ж�һ��

    /* �����ʼ�� */

    /* ���������� */
    Task_Process();
    while (1) {
    }
}

/***************** ������ȹ��� *****************/

/**
 * @brief  ������Ⱥ���
 * @note
 * @retval None
 */
void Task_Remarks(void) {
    uint16_t task_max = ArraySize(TaskList);   // ��������

    for (uint8_t i = 0; i < task_max; i++) {   // �������ʱ�䴦��
        if (TaskList[i].Cycle == 0) {          // �������Ϊ0
            continue;                          // ����������
        }
        if (TaskList[i].Timer > 0) {   // �����ʱ������0
            TaskList[i].Timer--;       // ��ʱ���ݼ�
        }
        if (TaskList[i].Timer == 0) {                // �����ʱ����0
            TaskList[i].Timer = TaskList[i].Cycle;   // �������ü�ʱ��
            TaskList[i].Ready = 1;                   // �������������־
        }
    }
}

/**
 * @brief  ��������
 * @note
 * @retval None
 */
void Task_Process(void) {
    uint16_t task_max = ArraySize(TaskList);   // ��������

    for (uint16_t i = 0; i < task_max; i++) {   // ��ʼ�������б�
        TaskList[i].Ready = 0;                  // ���������־
        TaskList[i].Timer = i + 1;              // ���ü�ʱ����ʼֵ
    }

    while (task_max) {
        for (uint16_t i = 0; i < task_max; i++) {   // ���������б�
            if ((TaskList[i].Ready != 0) ||         // ����������
                (TaskList[i].Cycle == 0)) {         // ��������Ϊ0
                TaskList[i].Hook();                 // ִ�������Ӻ���
                TaskList[i].Ready = 0;              // ���������־
            }
        }
    }
}

/**
 * @brief  ������
 * @note
 * @retval None
 */
void TaskNull(void) {
    return;
}

/***************** ��ʱ���� *****************/
uint32_t DelayTimer = 0;   // ��ʱ������

/**
 * @brief  ���к��뼶��ʱ
 * @note
 * @param  delay: ��ʱ�ĺ�����
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
