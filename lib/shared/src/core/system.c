#include "core/system.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/rcc.h>

static volatile uint64_t ticks=0;

uint64_t getTicks(void){
    return ticks;
}

void sys_tick_handler(void){
    ticks++;
}

static void rcc_setup(void){
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_osc_on(RCC_HSI);
    rcc_wait_for_osc_ready(RCC_HSI);
}

static void systick_setup(void){
    systick_set_frequency(SYS_FREQ,CPU_FREQ);
    systick_counter_enable();
    systick_interrupt_enable();
}

void system_setup(void){
    rcc_setup();
    systick_setup();
}

void system_teardown(void){
    systick_interrupt_disable();
    systick_counter_disable();
    systick_clear();
}

void system_delay(uint64_t millis){
    uint64_t endTime = getTicks() + millis;
    while(getTicks() < endTime){
        
    }
}
