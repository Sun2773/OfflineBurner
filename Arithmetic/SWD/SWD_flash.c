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
    if (FlashBlob->init == NULL) {
        return ERROR_FAILURE;
    }
    if (0 == swd_set_target_state_hw(RESET_PROGRAM)) {
        return ERROR_RESET;
    }

    // 下载编程算法到目标MCU的SRAM，并初始化
    if (0 == swd_write_memory(FlashBlob->algo_start, (uint8_t*) FlashBlob->algo_blob, FlashBlob->algo_size)) {
        return ERROR_ALGO_DL;
    }

    if (0 == swd_flash_syscall_exec(&FlashBlob->sys_call_s, FlashBlob->init, flash_start, 0, 0, 0)) {
        return ERROR_INIT;
    }

    return ERROR_SUCCESS;
}

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
                               0) == 0) {
        return ERROR_UINIT;
    }

    return ERROR_SUCCESS;
}

error_t target_flash_program_page(uint32_t addr, const uint8_t* buf, uint32_t size) {
    if (FlashBlob == NULL ||
        FlashBlob->program_page == NULL) {
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
    if (FlashBlob == NULL ||
        FlashBlob->erase_sector == NULL) {
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
    if (FlashBlob == NULL ||
        FlashBlob->erase_chip == NULL) {
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
                               0) == 0) {
        return ERROR_SET_RDP;
    }

    return ERROR_SUCCESS;
}
uint32_t target_flash_verify(uint32_t init, uint32_t addr, uint32_t size) {
    if (FlashBlob == NULL ||
        FlashBlob->verify == NULL) {
        return ERROR_FAILURE;
    }

    DEBUG_STATE state = {{0}, 0};
    // Call flash algorithm function on target and wait for result.
    state.r[0]  = init;                                  // R0: Argument 1
    state.r[1]  = size;                                  // R1: Argument 2
    state.r[2]  = addr;                                  // R2: Argument 3
    state.r[3]  = 0;                                     // R3: Argument 4
    state.r[9]  = FlashBlob->sys_call_s.static_base;     // SB: Static Base
    state.r[13] = FlashBlob->sys_call_s.stack_pointer;   // SP: Stack Pointer
    state.r[14] = FlashBlob->sys_call_s.breakpoint;      // LR: Exit Point
    state.r[15] = FlashBlob->set_rdp;                    // PC: Entry Point
    state.xpsr  = 0x01000000;                            // xPSR: T = 1, ISR = 0

    if (!swd_write_debug_state(&state)) {
        return 0;
    }

    if (!swd_wait_until_halted()) {
        return 0;
    }

    if (!swd_read_core_register(0, &state.r[0])) {
        return 0;
    }

    return state.r[0];
}

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