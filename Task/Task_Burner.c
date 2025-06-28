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

BurnerCtrl_t BurnerCtrl;

void Burner_Detection(void) {
    if (BurnerCtrl.Online != 0) {
        BurnerCtrl.Online = swd_read_idcode(&BurnerCtrl.ChipIdcode);
    } else {
        BurnerCtrl.Online = swd_init_debug();

        if (BurnerCtrl.Online != 0) {
            swd_read_memory(0xE000ED00, (void*) &BurnerCtrl.CPUID, 4);                                // 读取CPUID寄存器
            swd_read_memory(0xE0042000, (void*) &BurnerCtrl.DBGMCU_IDCODE, 4);                        // 读取DBGMCU IDCODE寄存器
            BurnerCtrl.FlashBlob = FlashBlob_Get(BurnerCtrl.DEV_ID & 0xFFF, 0);                       // 获取Flash编程算法
            swd_read_memory(BurnerCtrl.FlashBlob->FlashSizeAddr, (void*) &BurnerCtrl.FlashSize, 2);   // 读取Flash大小
            BurnerCtrl.FlashBlob = FlashBlob_Get(BurnerCtrl.DEV_ID & 0xFFF, BurnerCtrl.FlashSize);    // 获取Flash编程算法
        } else {
            BurnerCtrl.CPUID         = 0;
            BurnerCtrl.DBGMCU_IDCODE = 0;
            BurnerCtrl.FlashBlob     = NULL;
            BurnerCtrl.FlashSize     = 0;
            BurnerCtrl.ChipIdcode    = 0;
        }
    }

    switch (BurnerCtrl.State) {
        case BURNER_STATE_IDLE:
            if (BurnerCtrl.Online != 0) {
                if (BurnerCtrl.StartTimer > 0) {
                    BurnerCtrl.StartTimer--;
                } else {
                    BurnerCtrl.State = BURNER_STATE_READY;
                    if (BurnerConfigInfo.AutoBurner == 0) {
                        Beep(500);
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

void Burner_Exe(void) {
    if (BurnerCtrl.State != BURNER_STATE_START) {
        return;
    }
    BurnerCtrl.State = BURNER_STATE_RUNNING;
    BurnerCtrl.Error = 0;
    LED_Off(ERR);
    if (BurnerCtrl.Buffer == NULL) {
        BurnerCtrl.Buffer = pvPortMalloc(CONFIG_BUFFER_SIZE);
    }
    if (BurnerCtrl.Buffer == NULL) {
        return;   // 内存分配失败
    }
    Beep(150);
    uint32_t file_size   = BurnerConfigInfo.FileSize;      // 文件大小
    uint32_t file_finish = 0;                              // 已完成大小
    uint32_t rw_addr     = BurnerConfigInfo.FileAddress;   // 读写地址
    uint32_t rw_cnt      = 0;                              // 读写计数

    if (swd_init_debug() == 0) {
        BurnerCtrl.Error = 1;   // SWD初始化失败
        goto exit;              // 初始化失败
    }
    if (target_flash_init(BurnerCtrl.FlashBlob->prog_opt, 0) != ERROR_SUCCESS) {
        BurnerCtrl.Error = 2;   // 选项字初始化失败
        goto exit;              // 初始化失败
    }
    if (target_flash_erase_chip() != ERROR_SUCCESS) {
        BurnerCtrl.Error = 3;   // 选项字擦除失败
        goto exit;              // 擦除失败
    }
    target_flash_uninit();

    if (swd_init_debug() == 0) {
        BurnerCtrl.Error = 4;   // SWD初始化失败
        goto exit;              // 初始化失败
    }
    if (target_flash_init(BurnerCtrl.FlashBlob->prog_flash, 0x08000000) != ERROR_SUCCESS) {
        BurnerCtrl.Error = 5;   // Flash初始化失败
        goto exit;              // 初始化失败
    }
    if (BurnerConfigInfo.ChipErase != 0) {
        if (target_flash_erase_chip() != ERROR_SUCCESS) {
            BurnerCtrl.Error = 6;   // Flash擦除失败
            goto exit;              // 擦除失败
        }
    }
    while (file_size - file_finish) {
        if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
            /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
            rw_cnt = CONFIG_BUFFER_SIZE;
        } else {
            /* 剩余字节数大于0小于缓存,读取剩余字节数 */
            rw_cnt = (file_size - file_finish);
        }
        SPI_FLASH_Read(BurnerCtrl.Buffer, rw_addr + file_finish, CONFIG_BUFFER_SIZE);
        if ((BurnerConfigInfo.ChipErase == 0) &&
            (target_flash_sector_integer(BURNER_TARGET_ADDRESS + file_finish) != 0)) {
            if (target_flash_erase_sector(BURNER_TARGET_ADDRESS + file_finish) != ERROR_SUCCESS) {
                BurnerCtrl.Error = 7;   // Flash擦除失败
                break;                  // 擦除失败
            }
        }
        if (target_flash_program_page(BURNER_TARGET_ADDRESS + file_finish,
                                      BurnerCtrl.Buffer,
                                      rw_cnt) != ERROR_SUCCESS) {
            BurnerCtrl.Error = 8;   // Flash编程失败
            break;
        }
        file_finish += rw_cnt;
        LED_OnOff(RUN);
    }
    if (BurnerConfigInfo.AutoRun != 0) {
        if (swd_init_debug()) {
            swd_set_target_reset(0);   // 复位运行
        }
    }

    target_flash_uninit();
exit:
    if (BurnerCtrl.Error == 0) {
        Beep(300);
    } else {
        Beep(2000);
        LED_On(ERR);
    }
    BurnerCtrl.EndTimer = BURNER_AUTO_END_TIME;
    BurnerCtrl.State    = BURNER_STATE_FINISH;
}

void Burner_Task(void) {
    Burner_Detection();
    Burner_Exe();
}
