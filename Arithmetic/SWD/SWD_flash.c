/**
 * @file    SWD_flash.c
 * @brief   通过SWD协议对MCU的FLASH编程
 */
#include "SWD_flash.h"
#include "stdlib.h"
#include "swd_host.h"

static program_target_t* FlashBlob = NULL;

error_t target_flash_init(const program_target_t* prog, uint32_t flash_start) {
    FlashBlob = (program_target_t*) prog;

    if (0 == swd_set_target_state_hw(RESET_PROGRAM)) {
        return ERROR_RESET;
    }

    // 下载编程算法到目标MCU的SRAM，并初始化
    if (0 == swd_write_memory(prog->algo_start, (uint8_t*) prog->algo_blob, prog->algo_size)) {
        return ERROR_ALGO_DL;
    }

    if (0 == swd_flash_syscall_exec(&prog->sys_call_s, prog->init, flash_start, 0, 0, 0)) {
        return ERROR_INIT;
    }

    return ERROR_SUCCESS;
}

error_t target_flash_uninit(void) {
    if (FlashBlob == NULL) {
        return ERROR_FAILURE;
    }
    swd_set_target_state_hw(RESET_RUN);

    swd_off();
    return ERROR_SUCCESS;
}

error_t target_flash_program_page(uint32_t addr, const uint8_t* buf, uint32_t size) {
    if (FlashBlob == NULL) {
        return ERROR_FAILURE;
    }

    while (size > 0) {
        uint32_t write_size = size > FlashBlob->program_buffer_size ? FlashBlob->program_buffer_size : size;

        // Write page to buffer
        if (!swd_write_memory(FlashBlob->program_buffer, (uint8_t*) buf, write_size)) {
            return ERROR_ALGO_DATA_SEQ;
        }

        // Run flash programming
        if (!swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                                    FlashBlob->program_page,
                                    addr,
                                    write_size,
                                    FlashBlob->program_buffer,
                                    0)) {
            return ERROR_WRITE;
        }

        addr += write_size;
        buf += write_size;
        size -= write_size;
    }

    return ERROR_SUCCESS;
}

error_t target_flash_erase_sector(uint32_t addr) {
    if (FlashBlob == NULL) {
        return ERROR_FAILURE;
    }

    if (swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                               FlashBlob->erase_sector,
                               addr,
                               0,
                               0,
                               0) == 0) {
        return ERROR_ERASE_SECTOR;
    }

    return ERROR_SUCCESS;
}

error_t target_flash_erase_chip(void) {
    if (FlashBlob == NULL) {
        return ERROR_FAILURE;
    }
    error_t status = ERROR_SUCCESS;

    if (0 == swd_flash_syscall_exec(&FlashBlob->sys_call_s,
                                    FlashBlob->erase_chip,
                                    0,
                                    0,
                                    0,
                                    0)) {
        return ERROR_ERASE_ALL;
    }

    return status;
}

uint8_t target_flash_sector_integer(uint32_t addr) {
    if (FlashBlob == NULL) {
        return 0;
    }
    addr &= 0x07FFFFFF;

    for (uint32_t i = 0; i < FlashBlob->sector_info_count; i++) {
        if (addr >= FlashBlob->sector_info[i].AddrSector) {
            addr -= FlashBlob->sector_info[i].AddrSector;
            if ((addr % FlashBlob->sector_info[i].szSector) == 0) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}