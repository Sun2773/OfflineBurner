/**
 * @file    SWD_flash.c
 * @brief   通过SWD协议对MCU的FLASH编程
 */
#include "SWD_flash.h"
#include "stdlib.h"
#include "swd_host.h"

static program_target_t* FlashBlob = NULL;

/**
 * @brief  初始化目标Flash
 * @note   设置Flash编程算法并进行初始化
 * @param  prog: Flash编程目标结构体指针
 * @param  flash_start: Flash起始地址
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_init(const program_target_t* prog, uint32_t flash_start) {
    FlashBlob = (program_target_t*) prog;
    if (FlashBlob->init == NULL) {
        return ERROR_FAILURE;
    }
    if (swd_set_target_state_hw(RESET_PROGRAM) != 0) {
        return ERROR_RESET;
    }

    // 下载编程算法到目标MCU的SRAM，并初始化
    if (swd_write_memory(FlashBlob->algo_start,
                         (uint8_t*) FlashBlob->algo_blob,
                         FlashBlob->algo_size) != 0) {
        return ERROR_ALGO_DL;
    }

    if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                               FlashBlob->init,
                               flash_start,
                               0,
                               0,
                               0) != 0) {
        return ERROR_INIT;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief  反初始化目标Flash
 * @note   执行Flash编程完成后的清理工作
 * @param  None
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_uninit(void) {
    if (FlashBlob == NULL ||
        FlashBlob->uninit == NULL) {
        return ERROR_FAILURE;
    }

    if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                               FlashBlob->uninit,
                               0,
                               0,
                               0,
                               0) != 0) {
        return ERROR_UINIT;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief  编程Flash页面
 * @note   将数据写入到指定Flash地址
 * @param  addr: 目标Flash地址
 * @param  buf: 数据缓冲区指针
 * @param  size: 数据大小（字节）
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_program_page(uint32_t addr, const uint8_t* buf, uint32_t size) {
    if (FlashBlob == NULL ||
        FlashBlob->program_page == NULL) {
        return ERROR_FAILURE;
    }

    while (size > 0) {
        uint32_t write_size = size > FlashBlob->program_buffer_size ? FlashBlob->program_buffer_size : size;

        // Write page to buffer
        if (swd_write_memory(FlashBlob->program_buffer,
                             (uint8_t*) buf,
                             write_size) != 0) {
            return ERROR_ALGO_DATA_SEQ;
        }

        // Run flash programming
        if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                                   FlashBlob->program_page,
                                   addr,
                                   write_size,
                                   FlashBlob->program_buffer,
                                   0) != 0) {
            return ERROR_WRITE;
        }

        addr += write_size;
        buf += write_size;
        size -= write_size;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief  擦除Flash扇区
 * @note   擦除指定地址所在的Flash扇区
 * @param  addr: 目标Flash地址
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_erase_sector(uint32_t addr) {
    if (FlashBlob == NULL ||
        FlashBlob->erase_sector == NULL) {
        return ERROR_FAILURE;
    }

    if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                               FlashBlob->erase_sector,
                               addr,
                               0,
                               0,
                               0) != 0) {
        return ERROR_ERASE_SECTOR;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief  擦除整个Flash芯片
 * @note   擦除目标Flash的所有内容
 * @param  None
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_erase_chip(void) {
    if (FlashBlob == NULL ||
        FlashBlob->erase_chip == NULL) {
        return ERROR_FAILURE;
    }
    error_t status = ERROR_SUCCESS;

    if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                               FlashBlob->erase_chip,
                               0,
                               0,
                               0,
                               0) != 0) {
        return ERROR_ERASE_ALL;
    }

    return status;
}

/**
 * @brief  设置Flash读保护
 * @note   启用目标Flash的读保护功能
 * @param  None
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_set_rdp(void) {
    if (FlashBlob == NULL ||
        FlashBlob->set_rdp == NULL) {
        return ERROR_FAILURE;
    }

    if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                               FlashBlob->set_rdp,
                               0,
                               0,
                               0,
                               0) != 0) {
        return ERROR_SET_RDP;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief  验证Flash内容
 * @note   验证指定地址范围的Flash内容
 * @param  addr: 目标Flash地址
 * @param  size: 数据大小（字节）
 * @param  crc: CRC校验值
 * @retval ERROR_SUCCESS: 成功, 其他: 失败错误码
 */
error_t target_flash_verify(uint32_t addr, uint32_t size, uint32_t crc) {
    if (FlashBlob == NULL ||
        FlashBlob->verify == NULL) {
        return ERROR_FAILURE;
    }

    // Write page to buffer
    if (swd_write_memory(FlashBlob->program_buffer,
                         (uint8_t*) &crc,
                         4) != 0) {
        return ERROR_VERIFY;
    }
    uint32_t res = swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                                          FlashBlob->verify,
                                          addr,
                                          size,
                                          FlashBlob->program_buffer,
                                          0);
    if (res != (addr + size)) {
        return ERROR_VERIFY;
    }
    return ERROR_SUCCESS;
}

/**
 * @brief  检查地址是否为扇区整数倍
 * @note   检查给定地址是否对齐到扇区边界
 * @param  addr: 要检查的Flash地址
 * @retval 1: 是扇区整数倍, 0: 不是扇区整数倍
 */
uint8_t target_flash_sector_integer(uint32_t addr) {
    if (FlashBlob == NULL || FlashBlob->sector_info_count == 0) {
        return 0;
    }
    addr &= 0x07FFFFFF;
    uint8_t index = 0;
    while (index < FlashBlob->sector_info_count - 1) {
        if (addr >= FlashBlob->sector_info[index].AddrSector &&
            addr < FlashBlob->sector_info[index + 1].AddrSector) {
            break;
        }
        index++;
    }
    addr -= FlashBlob->sector_info[index].AddrSector;
    if ((addr % FlashBlob->sector_info[index].szSector) == 0) {
        return 1;
    }
    return 0;
}