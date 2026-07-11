#ifndef INC_FW_INFO_H
#define INC_FW_INFO_H

#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/vector.h>

#include "common-defines.h"

#define BOOTLOADER_SIZE     (0x2000U)
#define MAIN_APP_START_ADD  (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FLASH_APP_SIZE  ((1024U * 128U) - BOOTLOADER_SIZE)
#define DEVICE_ID           (0x42)

#define FW_INFO_SENTINEL      (0xDEADC0DE)
#define FW_INFO_START_ADD     (MAIN_APP_START_ADD + sizeof(vector_table_t))
#define FW_INFO_VALIDATE_FROM (FW_INFO_START_ADD + sizeof(firmware_info_t))

#define FW_INFO_VALIDATE_LENGTH(fw_length) (fw_length - (sizeof(vector_table_t) + sizeof(firmware_info_t)))

typedef struct firmware_info_t{
    uint32_t sentinel;
    uint32_t device_id;
    uint32_t version;
    uint32_t length;
    uint32_t reserved0;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t crc32;
}firmware_info_t;



#endif