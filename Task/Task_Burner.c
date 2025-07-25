#include "Task_Burner.h"

#include "BurnerConfig.h"
#include "DAP.h"
#include "FlashLayout.h"
#include "SPI_Flash.h"
#include "SWD_flash.h"
#include "SWD_host.h"
#include "buzzer.h"
#include "heap.h"
#include "hw_config.h"
#include "led.h"

extern uint32_t SysTick_Get(void);   // 获取系统滴答计数值
extern void     Delay(uint32_t);     // 获取系统滴答计数值

BurnerCtrl_t BurnerCtrl = {
    .StartTimer = BURNER_AUTO_START_TIME,
    .EndTimer   = BURNER_AUTO_END_TIME,
};

/**
 * @brief  检测目标
 * @note
 * @retval None
 */
void Burner_Detection(void) {
    if (BurnerCtrl.Online != 0) {
        BurnerCtrl.Online = (swd_read_idcode(&BurnerCtrl.Info.ChipIdcode) == 0);
    } else {
        BurnerCtrl.Online = (swd_init_debug() == 0);
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

        case BURNER_STATE_LOCK:
            if (USB_StateGet() == 0) {
                BurnerCtrl.State = BURNER_STATE_IDLE;
            }
            BurnerCtrl.StartTimer = BURNER_AUTO_START_TIME;
            BurnerCtrl.EndTimer   = BURNER_AUTO_END_TIME;
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
    uint8_t  rdp  = 0;
    if (USB_StateGet() != 0) {
        BurnerCtrl.State = BURNER_STATE_LOCK;
        return;
    }
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
    if (swd_init_debug() != 0) {
        BurnerCtrl.Error = BURNER_ERROR_INIT;   // SWD初始化失败
        goto exit;                              // 初始化失败
    }
    /* 读取idcode */
    if (swd_read_idcode(&BurnerCtrl.Info.ChipIdcode) != 0) {
        BurnerCtrl.Error = BURNER_ERROR_INIT;   // SWD初始化失败
        goto exit;                              // 初始化失败
    }
    /* 读取DBGMCU IDCODE寄存器 */
    if (swd_read_memory(0xE0042000, (void*) &BurnerCtrl.Info.DBGMCU_IDCODE, 4) != 0) {
        BurnerCtrl.Error = BURNER_ERROR_READ_FAIL;   // SWD初始化失败
        goto exit;                                   // 初始化失败
    }
    if (BurnerCtrl.Info.DEV_ID == 0) {
        if (swd_read_memory(0x40015800, (void*) &BurnerCtrl.Info.DBGMCU_IDCODE, 4) != 0) {
            BurnerCtrl.Error = BURNER_ERROR_READ_FAIL;   // SWD初始化失败
            goto exit;                                   // 初始化失败
        }
    }
    if (BurnerCtrl.Info.DEV_ID == 0) {
        BurnerCtrl.Error = BURNER_ERROR_CHIP_UNKNOWN;   // SWD初始化失败
        goto exit;                                      // 初始化失败
    }

    if (swd_read_memory(BurnerCtrl.FlashBlob->FlashSizeAddr,
                        (void*) &BurnerCtrl.Info.FlashSize,
                        2) != 0) {
        rdp++;   // 读取Flash大小失败 可能开启了rdp
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
        if (swd_init_debug() != 0) {
            continue;
        }
        LED_OnOff(RUN);
        /* 获取Flash大小 */
        if (swd_read_memory(BurnerCtrl.FlashBlob->FlashSizeAddr,
                            (void*) &BurnerCtrl.Info.FlashSize,
                            2) == 0) {
            rdp++;   // 读取Flash大小成功
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
    /* 发生了解除读保护，不需要擦除芯片了 */
    if (rdp < 2) {
        if (BurnerConfigInfo.ChipErase != 0) {
            /* 若配置了擦除全片则擦除全片 */
            LED_On(RUN);
            if (target_flash_erase_chip() != ERROR_SUCCESS) {
                BurnerCtrl.Error = BURNER_ERROR_FLASH_ERASE;   // Flash擦除失败
                goto exit;                                     // 擦除失败
            }
            LED_Off(RUN);
        } else {
            /* 擦除待编程的扇区 */
            for (uint32_t i = 0; i < BurnerCtrl.Info.ProgramSize; i += CONFIG_BUFFER_SIZE) {
                /* 擦除扇区 */
                if (target_flash_sector_integer(BurnerConfigInfo.FlashAddress + i) != 0) {
                    if (target_flash_erase_sector(BurnerConfigInfo.FlashAddress + i) != ERROR_SUCCESS) {
                        BurnerCtrl.Error = BURNER_ERROR_FLASH_ERASE;   // Flash擦除失败
                        goto exit;                                     // 擦除失败
                    }
                    LED_OnOff(RUN);
                }
            }
        }
    }
    /* 对Flash进行编程 */
    while (BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize) {
        uint32_t rw_cnt = 0;   // 读写计数
        if ((BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize) > CONFIG_BUFFER_SIZE) {
            /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
            rw_cnt = CONFIG_BUFFER_SIZE;
        } else {
            /* 剩余字节数大于0小于缓存,读取剩余字节数 */
            rw_cnt = (BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize);
        }
        SPI_FLASH_Read(BurnerCtrl.Buffer,
                       BurnerConfigInfo.FileAddress + BurnerCtrl.Info.FinishSize,
                       rw_cnt);
        /* 对Flash进行编程 */
        if (target_flash_program_page(
                BurnerConfigInfo.FlashAddress + BurnerCtrl.Info.FinishSize,
                BurnerCtrl.Buffer,
                rw_cnt) != ERROR_SUCCESS) {
            BurnerCtrl.Error = BURNER_ERROR_FLASH_PROGRAM;   // Flash编程失败
            break;
        }
        BurnerCtrl.Info.FinishSize += rw_cnt;
        BurnerCtrl.Info.FinishRate =
            BurnerCtrl.Info.FinishSize * 1000 / BurnerCtrl.Info.ProgramSize;
        LED_OnOff(RUN);
    }
    target_flash_uninit();

    /* 校验代码 */
    if (BurnerConfigInfo.Verify != 0) {
        BurnerCtrl.Info.FinishSize = 0;   // 重置已完成大小
        /* 对Flash内容进行校验 */
        while (BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize) {
            uint32_t rw_cnt = 0;   // 读写计数
            uint32_t f_addr = SPI_FLASH_VERIFY_ADDRESS;
            uint32_t t_addr = BurnerConfigInfo.FlashAddress;
            uint32_t crc    = 0;   // CRC校验码
            if ((BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize) > CONFIG_BUFFER_SIZE) {
                /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
                rw_cnt = CONFIG_BUFFER_SIZE;
            } else {
                /* 剩余字节数大于0小于缓存,读取剩余字节数 */
                rw_cnt = (BurnerCtrl.Info.ProgramSize - BurnerCtrl.Info.FinishSize);
            }
            f_addr += (BurnerCtrl.Info.FinishSize + CONFIG_BUFFER_SIZE - 1) /
                      CONFIG_BUFFER_SIZE * 4;       // Flash读取地址
            t_addr += BurnerCtrl.Info.FinishSize;   // Flash写入地址
            SPI_FLASH_Read(&crc, f_addr, 4);

            /* 对Flash进行编程 */
            if (target_flash_verify(t_addr, rw_cnt, crc) != ERROR_SUCCESS) {
                BurnerCtrl.Error = BURNER_ERROR_FLASH_VERIFY;   // Flash校验失败
                break;
            }
            BurnerCtrl.Info.FinishSize += rw_cnt;
            BurnerCtrl.Info.FinishRate = BurnerCtrl.Info.FinishSize * 1000 / BurnerCtrl.Info.ProgramSize;
            if ((BurnerCtrl.Info.FinishSize & 0x1000) == 0) {
                LED_OnOff(RUN);
            }
        }
    }

    /* 开启读保护 */
    if (BurnerConfigInfo.ReadProtection != 0) {
        /* 初始化选项字节编程算法 */
        if (target_flash_init(BurnerCtrl.FlashBlob->prog_opt, 0) != ERROR_SUCCESS) {
            BurnerCtrl.Error = BURNER_ERROR_OPT_INIT;   // 选项字初始化失败
            goto exit;                                  // 初始化失败
        }
        LED_On(RUN);
        /* 设置读保护 */
        if (target_flash_set_rdp() != ERROR_SUCCESS) {
            BurnerCtrl.Error = BURNER_ERROR_OTP_SETRDP;   // 选项字擦除失败
            goto exit;                                    // 擦除失败
        }
        LED_Off(RUN);
        /* 反初始化选项字节编程算法 */
        target_flash_uninit();
    }

    /* 编程完成，若配置了重启运行，则复位目标 */
    if (BurnerConfigInfo.AutoRun != 0) {
        if (swd_init_debug() == 0) {
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
