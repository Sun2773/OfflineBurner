/**
 * @file    SWD_opt.c
 * @brief   通过SWD协议对MCU的FLASH编程
 */
#include "SWD_opt.h"
#include "swd_host.h"

extern const program_target_t _stm32f10x_opt_;

error_t target_opt_init(void) {
    if (0 == swd_set_target_state_hw(RESET_PROGRAM)) {
        return ERROR_RESET;
    }

    // 下载编程算法到目标MCU的SRAM，并初始化
    if (0 == swd_write_memory(_stm32f10x_opt_.algo_start, (uint8_t*) _stm32f10x_opt_.algo_blob, _stm32f10x_opt_.algo_size)) {
        return ERROR_ALGO_DL;
    }

    if (0 == swd_flash_syscall_exec(&_stm32f10x_opt_.sys_call_s, _stm32f10x_opt_.init, 0, 0, 0, 0)) {
        return ERROR_INIT;
    }

    return ERROR_SUCCESS;
}

error_t target_opt_uninit(void) {
    if (0 == swd_flash_syscall_exec(&_stm32f10x_opt_.sys_call_s, _stm32f10x_opt_.uninit, 0, 0, 0, 0)) {
        return ERROR_INIT;
    }
    return ERROR_SUCCESS;
}

error_t target_opt_program_page(uint32_t addr, const uint8_t* buf, uint32_t size) {
    // Write page to buffer
    if (!swd_write_memory(_stm32f10x_opt_.program_buffer, (uint8_t*) buf, size)) {
        return ERROR_ALGO_DATA_SEQ;
    }

    // Run flash programming
    if (!swd_flash_syscall_exec(&_stm32f10x_opt_.sys_call_s,
                                _stm32f10x_opt_.program_page,
                                addr,
                                size,
                                _stm32f10x_opt_.program_buffer,
                                0)) {
        return ERROR_WRITE;
    }
    return ERROR_SUCCESS;
}

error_t target_opt_erase_sector(uint32_t addr) {
    if (0 == swd_flash_syscall_exec(&_stm32f10x_opt_.sys_call_s, _stm32f10x_opt_.erase_sector, addr, 0, 0, 0)) {
        return ERROR_ERASE_SECTOR;
    }

    return ERROR_SUCCESS;
}

error_t target_opt_erase_chip(void) {
    error_t status = ERROR_SUCCESS;

    if (0 == swd_flash_syscall_exec(&_stm32f10x_opt_.sys_call_s, _stm32f10x_opt_.erase_chip, 0, 0, 0, 0)) {
        return ERROR_ERASE_ALL;
    }

    return status;
}
