#include "Task_Burner.h"

#include "BurnerConfig.h"
#include "DAP.h"
#include "SPI_Flash.h"
#include "SWD_flash.h"
#include "SWD_host.h"
#include "buzzer.h"
#include "heap.h"
#include "led.h"

extern uint32_t SysTick_Get(void);   // 获取系统滴答计数值
extern void     Delay(uint32_t);     // 获取系统滴答计数值

BurnerCtrl_t BurnerCtrl;

/**
 * @brief  检测目标
 * @note
 * @retval None
 */
void Burner_Detection(void) {
    if (BurnerCtrl.Online != 0) {
        BurnerCtrl.Online = swd_read_idcode(&BurnerCtrl.Info.ChipIdcode);
    } else {
        BurnerCtrl.Online = swd_init_debug();
    }

    switch (BurnerCtrl.State) {
        case BURNER_STATE_IDLE:
            if (BurnerCtrl.Online != 0) {
                if (BurnerCtrl.StartTimer > 0) {
                    BurnerCtrl.StartTimer--;
                } else {
                    BurnerCtrl.State = BURNER_STATE_READY;
                    if (BurnerConfigInfo.AutoBurner == 0) {
                        Beep(50);
                    }
                }
            } else {
                BurnerCtrl.StartTimer = BURNER_AUTO_START_TIME;
            }
            BurnerCtrl.EndTimer = BURNER_AUTO_END_TIME;
            break;
        case BURNER_STATE_READY:
            if (BurnerCtrl.Online == 0) {
                BurnerCtrl.State = BURNER_STATE_IDLE;   // 如果不在线, 切换到空闲状态
            } else if (BurnerConfigInfo.AutoBurner != 0) {
                BurnerCtrl.State = BURNER_STATE_START;
            }
            break;
        case BURNER_STATE_FINISH:
            if (BurnerCtrl.Online == 0) {
                if (BurnerCtrl.EndTimer > 0) {
                    BurnerCtrl.EndTimer--;
                } else {
                    BurnerCtrl.State = BURNER_STATE_IDLE;
                }
            } else {
                BurnerCtrl.EndTimer = BURNER_AUTO_END_TIME;
            }
            break;
    }
}

/**
 * @brief  执行烧录
 * @note
 * @retval None
 */
void Burner_Exe(void) {
    uint32_t tick = SysTick_Get();
    /* 等待开始命令 */
    if (BurnerCtrl.State != BURNER_STATE_START) {
        return;
    }
    /* 复位编程信息 */
    BurnerCtrl.State = BURNER_STATE_RUNNING;
    BurnerCtrl.Error = BURNER_ERROR_NONE;
    memset(&BurnerCtrl.Info, 0, sizeof(BurnerCtrl.Info));

    LED_Off(ERR);
    /* 分配缓存 */
    if (BurnerCtrl.Buffer == NULL) {
        if ((BurnerCtrl.Buffer = pvPortMalloc(CONFIG_BUFFER_SIZE)) == NULL) {
            BurnerCtrl.Error = BURNER_ERROR_BUFFER;   // 缓存分配失败
            goto exit;                                // 初始化失败
        }
    }
    /* 蜂鸣器短鸣 */
    Beep(150);
    /* 初始化接口 */
    if (swd_init_debug() == 0) {
        BurnerCtrl.Error = BURNER_ERROR_INIT;   // SWD初始化失败
        goto exit;                              // 初始化失败
    }
    /* 读取idcode */
    if (swd_read_idcode(&BurnerCtrl.Info.ChipIdcode) == 0) {
        BurnerCtrl.Error = BURNER_ERROR_INIT;   // SWD初始化失败
        goto exit;                              // 初始化失败
    }
    /* 读取DBGMCU IDCODE寄存器 */
    if (swd_read_memory(0xE0042000, (void*) &BurnerCtrl.Info.DBGMCU_IDCODE, 4) == 0) {
        BurnerCtrl.Error = BURNER_ERROR_READ_FAIL;   // SWD初始化失败
        goto exit;                                   // 初始化失败
    }
    if (BurnerCtrl.Info.DEV_ID == 0) {
        if (swd_read_memory(0x40015800, (void*) &BurnerCtrl.Info.DBGMCU_IDCODE, 4) == 0) {
            BurnerCtrl.Error = BURNER_ERROR_READ_FAIL;   // SWD初始化失败
            goto exit;                                   // 初始化失败
        }
    }
    if (BurnerCtrl.Info.DEV_ID == 0) {
        BurnerCtrl.Error = BURNER_ERROR_CHIP_UNKNOWN;   // SWD初始化失败
        goto exit;                                      // 初始化失败
    }
    /* 初步匹配编程算法 */
    BurnerCtrl.FlashBlob = FlashBlob_Get(BurnerCtrl.Info.DEV_ID & 0xFFF, 0);
    if (BurnerCtrl.FlashBlob == NULL) {
        BurnerCtrl.Error = BURNER_ERROR_CHIP_UNKNOWN;   // SWD初始化失败
        goto exit;                                      // 初始化失败
    }
    /* 初始化选项字节编程算法 */
    if (target_flash_init(BurnerCtrl.FlashBlob->prog_opt, 0) != ERROR_SUCCESS) {
        BurnerCtrl.Error = BURNER_ERROR_OPT_INIT;   // 选项字初始化失败
        goto exit;                                  // 初始化失败
    }
    LED_On(RUN);
    /* 复位选项字节 */
    if (target_flash_erase_chip() != ERROR_SUCCESS) {
        BurnerCtrl.Error = BURNER_ERROR_OPT_ERASE;   // 选项字擦除失败
        goto exit;                                   // 擦除失败
    }
    LED_Off(RUN);
    /* 反初始化选项字节编程算法 */
    target_flash_uninit();

    /* 等待响应 */
    for (uint16_t i = 0; i < 100; i++) {
        Delay(10);
        /* 初始化接口 */
        if (swd_init_debug() == 0) {
            continue;
        }
        if (0 == swd_set_target_state_hw(RESET_PROGRAM)) {
            continue;
        }
        LED_OnOff(RUN);
        /* 获取Flash大小 */
        if (swd_read_memory(BurnerCtrl.FlashBlob->FlashSizeAddr, (void*) &BurnerCtrl.Info.FlashSize, 2) != 0) {   // 读取Flash大小
            break;
        }
    }
    if ((BurnerCtrl.Info.FlashSize == 0) ||
        (BurnerCtrl.Info.FlashSize == 0xFFFF)) {
        BurnerCtrl.Error = BURNER_ERROR_FLASH_SIZE;   // 读取Flash大小失败
        goto exit;                                    // 初始化失败
    }
    /* 重新匹配编程算法 */
    BurnerCtrl.FlashBlob = FlashBlob_Get(BurnerCtrl.Info.DEV_ID & 0xFFF, BurnerCtrl.Info.FlashSize);   // 获取Flash编程算法
    /* 算法错误 */
    if (BurnerCtrl.FlashBlob == NULL ||
        BurnerConfigInfo.FileSize == 0 ||
        BurnerConfigInfo.FileAddress == 0) {
        BurnerCtrl.Error = BURNER_ERROR_FLASH_ALGO;
        goto exit;
    }
    /* 获取文件大小 */
    BurnerCtrl.Info.ProgramSize = BurnerConfigInfo.FileSize;
    /* 初始化Flash编程算法 */
    if (target_flash_init(BurnerCtrl.FlashBlob->prog_flash, 0x08000000) != ERROR_SUCCESS) {
        BurnerCtrl.Error = BURNER_ERROR_FLASH_INIT;   // Flash初始化失败
        goto exit;                                    // 初始化失败
    }
    /* 若配置了擦除全片则擦除全片 */
    if (BurnerConfigInfo.ChipErase != 0) {
        if (target_flash_erase_chip() != ERROR_SUCCESS) {
            BurnerCtrl.Error = BURNER_ERROR_FLASH_ERASE;   // Flash擦除失败
            goto exit;                                     // 擦除失败
        }
    }
    /* 对Flash进行编程 */
    while (BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize) {
        uint32_t rw_cnt = 0;   // 读写计数
        uint32_t f_addr = BurnerConfigInfo.FileAddress;
        uint32_t t_addr = BURNER_TARGET_ADDRESS;
        if ((BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize) > CONFIG_BUFFER_SIZE) {
            /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
            rw_cnt = CONFIG_BUFFER_SIZE;
        } else {
            /* 剩余字节数大于0小于缓存,读取剩余字节数 */
            rw_cnt = (BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize);
        }
        f_addr += BurnerCtrl.Info.FinishSize;   // Flash读取地址
        t_addr += BurnerCtrl.Info.FinishSize;   // Flash写入地址
        SPI_FLASH_Read(BurnerCtrl.Buffer, f_addr, rw_cnt);
        /* 若未配置擦除全片，到达扇区起始地址，擦除扇区 */
        if ((BurnerConfigInfo.ChipErase == 0) &&
            (target_flash_sector_integer(t_addr) != 0)) {
            LED_On(RUN);
            if (target_flash_erase_sector(t_addr) != ERROR_SUCCESS) {
                BurnerCtrl.Error = BURNER_ERROR_FLASH_ERASE;   // Flash擦除失败
                break;                                         // 擦除失败
            }
            LED_Off(RUN);
        }
        /* 对Flash进行编程 */
        if (target_flash_program_page(t_addr, BurnerCtrl.Buffer, rw_cnt) != ERROR_SUCCESS) {
            BurnerCtrl.Error = BURNER_ERROR_FLASH_PROGRAM;   // Flash编程失败
            break;
        }
        BurnerCtrl.Info.FinishSize += rw_cnt;
        BurnerCtrl.Info.FinishRate = BurnerCtrl.Info.FinishSize * 1000 / BurnerCtrl.Info.ProgramSize;
        LED_OnOff(RUN);
    }
    target_flash_uninit();

    /* 编程完成，若配置了重启运行，则复位目标 */
    if (BurnerConfigInfo.AutoRun != 0) {
        if (swd_init_debug()) {
            swd_set_target_reset(0);   // 复位运行
        }
    }

exit:
    if (BurnerCtrl.Error == BURNER_ERROR_NONE) {
        Beep(300);
    } else {
        Beep(2000);
        LED_On(ERR);
    }
    BurnerCtrl.EndTimer        = BURNER_AUTO_END_TIME;
    BurnerCtrl.State           = BURNER_STATE_FINISH;
    BurnerCtrl.Info.FinishTime = SysTick_Get() - tick;   // 计算完成时间
}

/**
 * @brief  编程烧录任务
 * @note   100ms执行一次
 * @retval None
 */
void Burner_Task(void) {
    Burner_Detection();
    Burner_Exe();
}
