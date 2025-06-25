#ifndef __CRC_H__
#define __CRC_H__

#include "stdint.h"

uint32_t CRC32_Update(uint32_t init, void* buff, uint32_t length);
uint16_t CRC16_Modbus(void* buff, uint16_t len);

#endif
