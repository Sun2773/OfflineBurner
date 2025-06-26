/**
 * @file    SWD_opt.c
 * @brief   通过SWD协议对MCU的FLASH编程
 */
#include "SWD_opt.h"
#include "swd_host.h"

#include "Task_Burner.h"

error_t target_opt_init(void) {
    if (BurnerCtrl.FlashBlob == NULL ||
        BurnerCtrl.FlashBlob->prog_opt == NULL) {
        return ERROR_FAILURE;
    }
    program_target_t* prog = (program_target_t*) BurnerCtrl.FlashBlob->prog_opt;

    if (0 == swd_set_target_state_hw(RESET_PROGRAM)) {
        return ERROR_RESET;
    }

    // 下载编程算法到目标MCU的SRAM，并初始化
    if (0 == swd_write_memory(prog->algo_start, (uint8_t*) prog->algo_blob, prog->algo_size)) {
        return ERROR_ALGO_DL;
    }

    if (0 == swd_flash_syscall_exec(&prog->sys_call_s, prog->init, 0, 0, 0, 0)) {
        return ERROR_INIT;
    }

    return ERROR_SUCCESS;
}

error_t target_opt_uninit(void) {
    if (BurnerCtrl.FlashBlob == NULL ||
        BurnerCtrl.FlashBlob->prog_opt == NULL) {
        return ERROR_FAILURE;
    }
    program_target_t* prog = (program_target_t*) BurnerCtrl.FlashBlob->prog_opt;

    if (0 == swd_flash_syscall_exec(&prog->sys_call_s, prog->uninit, 0, 0, 0, 0)) {
        return ERROR_INIT;
    }
    return ERROR_SUCCESS;
}

error_t target_opt_program_page(uint32_t addr, const uint8_t* buf, uint32_t size) {
    if (BurnerCtrl.FlashBlob == NULL ||
        BurnerCtrl.FlashBlob->prog_opt == NULL) {
        return ERROR_FAILURE;
    }
    program_target_t* prog = (program_target_t*) BurnerCtrl.FlashBlob->prog_opt;

    // Write page to buffer
    if (!swd_write_memory(prog->program_buffer, (uint8_t*) buf, size)) {
        return ERROR_ALGO_DATA_SEQ;
    }

    // Run flash programming
    if (!swd_flash_syscall_exec(&prog->sys_call_s,
                                prog->program_page,
                                addr,
                                size,
                                prog->program_buffer,
                                0)) {
        return ERROR_WRITE;
    }
    return ERROR_SUCCESS;
}

error_t target_opt_erase_sector(uint32_t addr) {
    if (BurnerCtrl.FlashBlob == NULL ||
        BurnerCtrl.FlashBlob->prog_opt == NULL) {
        return ERROR_FAILURE;
    }
    program_target_t* prog = (program_target_t*) BurnerCtrl.FlashBlob->prog_opt;

    if (0 == swd_flash_syscall_exec(&prog->sys_call_s, prog->erase_sector, addr, 0, 0, 0)) {
        return ERROR_ERASE_SECTOR;
    }

    return ERROR_SUCCESS;
}

error_t target_opt_erase_chip(void) {
    if (BurnerCtrl.FlashBlob == NULL ||
        BurnerCtrl.FlashBlob->prog_opt == NULL) {
        return ERROR_FAILURE;
    }
    program_target_t* prog = (program_target_t*) BurnerCtrl.FlashBlob->prog_opt;

    error_t status = ERROR_SUCCESS;

    if (0 == swd_flash_syscall_exec(&prog->sys_call_s, prog->erase_chip, 0, 0, 0, 0)) {
        return ERROR_ERASE_ALL;
    }

    return status;
}
