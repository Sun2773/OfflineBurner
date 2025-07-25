#ifndef __SWD_FLASH_H__
#define __SWD_FLASH_H__

#include <stdint.h>
#include "flash_blob.h"

typedef enum {
    /* Shared errors */
    ERROR_SUCCESS = 0,
    ERROR_FAILURE,
    ERROR_INTERNAL,

    /* Target flash errors */
    ERROR_RESET,
    ERROR_ALGO_DL,
    ERROR_ALGO_DATA_SEQ,
    ERROR_INIT,
    ERROR_UINIT,
    ERROR_SECURITY_BITS,
    ERROR_UNLOCK,
    ERROR_ERASE_SECTOR,
    ERROR_ERASE_ALL,
    ERROR_WRITE,
    ERROR_SET_RDP,
    ERROR_VERIFY,
    // Add new values here

    ERROR_COUNT
} error_t;

error_t target_flash_init(const program_target_t* prog, uint32_t flash_start);
error_t target_flash_uninit(void);
error_t target_flash_program_page(uint32_t addr, const uint8_t* buf, uint32_t size);
error_t target_flash_erase_sector(uint32_t addr);
error_t target_flash_erase_chip(void);
error_t target_flash_set_rdp(void);
error_t target_flash_verify(uint32_t addr, uint32_t size, uint32_t crc);
uint8_t target_flash_sector_integer(uint32_t addr);

#endif   // __SWD_FLASH_H__
