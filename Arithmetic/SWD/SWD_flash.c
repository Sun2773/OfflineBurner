/**
 * @file    SWD_flash.c
 * @brief   通过SWD协议对MCU的FLASH编程
 */
#include "SWD_flash.h"
#include "swd_host.h"

extern const program_target_t _stm32f10x_flash_;

error_t target_flash_init(uint32_t flash_start) {
    if (0 == swd_set_target_state_hw(RESET_PROGRAM)) {
        return ERROR_RESET;
    }

    // 下载编程算法到目标MCU的SRAM，并初始化
    if (0 == swd_write_memory(_stm32f10x_flash_.algo_start, (uint8_t*) _stm32f10x_flash_.algo_blob, _stm32f10x_flash_.algo_size)) {
        return ERROR_ALGO_DL;
    }

    if (0 == swd_flash_syscall_exec(&_stm32f10x_flash_.sys_call_s, _stm32f10x_flash_.init, flash_start, 0, 0, 0)) {
        return ERROR_INIT;
    }

    return ERROR_SUCCESS;
}

error_t target_flash_uninit(void) {
    swd_set_target_state_hw(RESET_RUN);

    swd_off();
    return ERROR_SUCCESS;
}

error_t target_flash_program_page(uint32_t addr, const uint8_t* buf, uint32_t size) {
    while (size > 0) {
        uint32_t write_size = size > _stm32f10x_flash_.program_buffer_size ? _stm32f10x_flash_.program_buffer_size : size;

        // Write page to buffer
        if (!swd_write_memory(_stm32f10x_flash_.program_buffer, (uint8_t*) buf, write_size)) {
            return ERROR_ALGO_DATA_SEQ;
        }

        // Run flash programming
        if (!swd_flash_syscall_exec(&_stm32f10x_flash_.sys_call_s,
                                    _stm32f10x_flash_.program_page,
                                    addr,
                                    _stm32f10x_flash_.program_buffer_size,
                                    _stm32f10x_flash_.program_buffer,
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
    if (0 == swd_flash_syscall_exec(&_stm32f10x_flash_.sys_call_s, _stm32f10x_flash_.erase_sector, addr, 0, 0, 0)) {
        return ERROR_ERASE_SECTOR;
    }

    return ERROR_SUCCESS;
}

error_t target_flash_erase_chip(void) {
    error_t status = ERROR_SUCCESS;

    if (0 == swd_flash_syscall_exec(&_stm32f10x_flash_.sys_call_s, _stm32f10x_flash_.erase_chip, 0, 0, 0, 0)) {
        return ERROR_ERASE_ALL;
    }

    return status;
}
