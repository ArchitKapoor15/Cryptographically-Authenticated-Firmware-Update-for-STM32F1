#ifndef INC_FW_INFO_H
#define INC_FW_INFO_H

#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/vector.h>

#include "common-defines.h"

#define ALIGNED(address,align) (((address) - 1U + (align)) & -(align))

#define BOOTLOADER_SIZE     (0x2000U)
#define MAIN_APP_START_ADD  (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FLASH_APP_SIZE  ((1024U * 128U) - BOOTLOADER_SIZE)
#define DEVICE_ID           (0x42)

#define FW_INFO_SENTINEL      (0xDEADC0DE)
#define FW_INFO_START_ADD     (ALIGNED((MAIN_APP_START_ADD + sizeof(vector_table_t)),16))
#define SIGNATURE_ADDRESS     (FW_INFO_START_ADD + sizeof(firmware_info_t))

typedef struct firmware_info_t{
    uint32_t sentinel;
    uint32_t device_id;
    uint32_t version;
    uint32_t length;
}firmware_info_t;

#endif