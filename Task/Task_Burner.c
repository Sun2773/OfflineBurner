#include "Task_Burner.h"

#include "BurnerConfig.h"
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
    }
}

// static void Burner_Exe(void) {
//     uint8_t res = 0;

//     if (BurnerCtrl.Start == 0) {
//         return;
//     }
//     if (BurnerCtrl.Buffer == NULL) {
//         BurnerCtrl.Buffer = pvPortMalloc(CONFIG_BUFFER_SIZE);
//     }
//     if (BurnerCtrl.Buffer == NULL) {
//         return;   // 内存分配失败
//     }
//     /* 初始化目标 */
//     if (swd_init_debug() == 0) {
//         return;
//     }
//     /* 初始化目标闪存 */
//     if (target_opt_init() != ERROR_SUCCESS) {
//         return;
//     }
//     /* 初始化Flash */
//     if (target_flash_init(0x08000000) != ERROR_SUCCESS) {
//         return;
//     }
//     /* 擦除全片 */
//     if (target_opt_erase_chip() != ERROR_SUCCESS) {
//         return;
//     }

//     uint32_t file_size   = BurnerConfigInfo.FileSize;      // 文件大小
//     uint32_t file_finish = 0;                              // 已完成大小
//     uint32_t rw_addr     = BurnerConfigInfo.FileAddress;   // 读写地址
//     uint32_t rw_cnt      = 0;                              // 读写计数

//     /* 开始复制文件 */
//     while (file_size - file_finish) {
//         if ((file_size - file_finish) > CONFIG_BUFFER_SIZE) {
//             /* 检查剩余字节数,若剩余字节大于缓存,读取缓存大小文件 */
//             rw_cnt = CONFIG_BUFFER_SIZE;
//         } else {
//             /* 剩余字节数大于0小于缓存,读取剩余字节数 */
//             rw_cnt = (file_size - file_finish);
//         }
//         /* 读取文件 */
//         rw_addr = FLASH_PROGRAM_ADDRESS + file_finish;   // 写入地址
//         SPI_FLASH_Read(BurnerCtrl.Buffer, rw_addr, rw_cnt);
//         if (target_flash_program_page(BURNER_TARGET_ADDRESS + file_finish,
//                                       BurnerCtrl.Buffer,
//                                       rw_cnt) != ERROR_SUCCESS) {
//             break;
//         }
//         /* 计数 */
//         file_finish += rw_cnt;
//         LED_OnOff(RUN);
//     }
//     if (swd_init_debug()) {
//         swd_set_target_reset(0);   // 复位运行
//     }
// }

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
