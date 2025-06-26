#include "Task_Burner.h"

#include "BurnerConfig.h"
#include "DAP.h"
#include "SPI_Flash.h"
#include "SWD_flash.h"
#include "SWD_host.h"
#include "SWD_opt.h"
#include "error.h"
#include "heap.h"
#include "led.h"

BurnerCtrl_t BurnerCtrl;

static void Burner_Detection(void) {
    if (BurnerCtrl.Start != 0) {
        return;
    }
    if (BurnerCtrl.Online != 0) {
        BurnerCtrl.Online = swd_read_idcode(&BurnerCtrl.ChipIdcode);
    } else {
        BurnerCtrl.ChipIdcode = 0;
        BurnerCtrl.Online     = swd_init_debug();
        if (BurnerCtrl.Online != 0) {
            swd_read_memory(0xE000ED00, (void*) &BurnerCtrl.CPUID, 4);                                // 读取CPUID寄存器
            swd_read_memory(0xE0042000, (void*) &BurnerCtrl.DBGMCU_IDCODE, 4);                        // 读取DBGMCU IDCODE寄存器
            BurnerCtrl.FlashBlob = FlashBlob_GetForId(BurnerCtrl.DEV_ID & 0xFFF);                     // 获取Flash编程算法
            swd_read_memory(BurnerCtrl.FlashBlob->FlashSizeAddr, (void*) &BurnerCtrl.FlashSize, 2);   // 读取Flash大小
        } else {
            BurnerCtrl.CPUID         = 0;
            BurnerCtrl.DBGMCU_IDCODE = 0;
            BurnerCtrl.FlashBlob     = NULL;
            BurnerCtrl.FlashSize     = 0;
        }
    }
}

static void Burner_Exe(void) {
    if (BurnerCtrl.Start != 1) {
        return;
    }
    BurnerCtrl.Start = 2;
    if (BurnerCtrl.Buffer == NULL) {
        BurnerCtrl.Buffer = pvPortMalloc(CONFIG_BUFFER_SIZE);
    }
    if (BurnerCtrl.Buffer == NULL) {
        return;   // 内存分配失败
    }

    uint32_t file_size   = BurnerConfigInfo.FileSize;      // 文件大小
    uint32_t file_finish = 0;                              // 已完成大小
    uint32_t rw_addr     = BurnerConfigInfo.FileAddress;   // 读写地址
    uint32_t rw_cnt      = 0;                              // 读写计数

    if (swd_init_debug()) {
        if (target_opt_init() == ERROR_SUCCESS) {
            if (target_opt_erase_chip() != ERROR_SUCCESS) {
                return;
            }
        } else {
            return;
        }
        target_opt_uninit();
        if (swd_init_debug()) {
            if (target_flash_init(0x08000000) == ERROR_SUCCESS) {
                if (target_flash_erase_chip() == ERROR_SUCCESS) {
                    while (file_size - file_finish) {
                        if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
                            /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
                            rw_cnt = CONFIG_BUFFER_SIZE;
                        } else {
                            /* 剩余字节数大于0小于缓存,读取剩余字节数 */
                            rw_cnt = (file_size - file_finish);
                        }
                        SPI_FLASH_Read(BurnerCtrl.Buffer, rw_addr + file_finish, CONFIG_BUFFER_SIZE);
                        if (target_flash_program_page(BURNER_TARGET_ADDRESS + file_finish,
                                                      BurnerCtrl.Buffer,
                                                      rw_cnt) == ERROR_SUCCESS) {
                        } else {
                            return;
                        }
                        file_finish += rw_cnt;
                        LED_OnOff(RUN);
                    }
                    if (swd_init_debug()) {
                        swd_set_target_reset(0);   // 复位运行
                        return;
                    } else {
                        return;
                    }
                } else {
                    return;
                }
            }
            target_flash_uninit();
        } else {
            return;
        }
    } else {
        return;
    }
}

void Burner_Task(void) {
    Burner_Detection();
    Burner_Exe();
}
