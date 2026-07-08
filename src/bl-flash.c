#include <libopencm3/stm32/flash.h>
#include "bl-flash.h"

#define APP_START_PAGE (0x08002000)
#define APP_END_PAGE   (0x08020000)
#define PAGE_SIZE      (0x400)

void bl_flash_erase_app(void){
    flash_unlock();

    for(uint32_t addr = APP_START_PAGE; addr < APP_END_PAGE; addr += PAGE_SIZE){
        flash_erase_page(addr);
    }

    flash_lock();
}

void bl_flash_write(const uint32_t address, const uint8_t* data, const uint32_t length){
    flash_unlock();
    
    for(uint32_t i=0; i<length; i+=2){
        uint16_t halfword = ((uint16_t)data[i+1]<<8) | (uint16_t)data[i];
        flash_program_half_word(address+i,halfword);
    }

    flash_lock();
}
