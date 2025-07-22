#include "crc.h"

// CRC32多项式 (IEEE 802.3标准): 0x04C11DB7
#define CRC32_POLYNOMIAL 0xEDB88320UL  // 反向多项式

uint32_t CRC32_Update(uint32_t init, void* buff, uint32_t length) {
    uint8_t* data = buff;
    uint32_t crc = init ^ 0xFFFFFFFF;
    
    if (data != 0) {
        for (uint32_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
                } else {
                    crc >>= 1;
                }
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 *CRC16-Modbus
 */
uint16_t CRC16_Modbus(void* buff, uint16_t len) {
    uint16_t crc  = 0xFFFF;
    uint8_t* data = buff;
    for (uint16_t j = 0; j < len; j++) {
        crc = crc ^ *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if ((crc & 0x0001) > 0) {
                crc = crc >> 1;
                crc = crc ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    uint8_t t = (uint8_t) ((crc & 0xFF00) >> 8);
    crc <<= 8;
    crc |= t;
    return crc;
}
