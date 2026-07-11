#include "core/fw-info.h"

__attribute__ ((section(".firmware_info")))

firmware_info_t fw_info = {
    .sentinel  = FW_INFO_SENTINEL,
    .device_id = DEVICE_ID,
    .version   = 0xFFFFFFFF,
    .length    = 0xFFFFFFFF,
    .reserved0 = 0xFFFFFFFF,
    .reserved1 = 0xFFFFFFFF,
    .reserved2 = 0xFFFFFFFF,
    .reserved3 = 0xFFFFFFFF,
    .reserved4 = 0xFFFFFFFF,
    .crc32     = 0xFFFFFFFF
};