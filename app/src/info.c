#include "core/fw-info.h"

__attribute__ ((section(".firmware_info")))

firmware_info_t fw_info = {
    .sentinel  = FW_INFO_SENTINEL,
    .device_id = DEVICE_ID,
    .version   = 0xFFFFFFFF,
    .length    = 0xFFFFFFFF,
};

__attribute__ ((section(".firmware_signature")))

uint8_t fw_signature[16] = {0};
