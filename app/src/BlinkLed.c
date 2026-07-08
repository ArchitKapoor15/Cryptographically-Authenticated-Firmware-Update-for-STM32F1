#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#include "core/system.h"
#include "core/uart.h"
#include "timer.h"

#define BOOTLOADER_SIZE (0x2000U)

#define LED_PORT GPIOC
#define LED_PIN GPIO13

#define UART_PORT GPIOA
#define TX_PIN GPIO9
#define RX_PIN GPIO10

static void vector_setup(void){
    SCB_VTOR = BOOTLOADER_SIZE;
}

static void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(LED_PORT,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_PUSHPULL,LED_PIN);
    
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(UART_PORT,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,TX_PIN);
    gpio_set_mode(UART_PORT,GPIO_MODE_INPUT,GPIO_CNF_INPUT_FLOAT,RX_PIN);
}

// static void delay_cycles(uint32_t delay){
//     for(uint32_t i=0;i<delay;i++){
//         __asm__("nop");
//     }
// }

int main(void){
    // rcc_setup();
    // systick_setup();
    vector_setup();
    system_setup();
    uart_setup();
    gpio_setup();
    timer_setup();

    uint64_t startTime = getTicks();

    while(1){
        if(getTicks()-startTime >= 1000){
            gpio_toggle(LED_PORT,LED_PIN);
            startTime = getTicks();
        }
        // delay_cycles(72000000/4);

        while(uart_data_available()){
            uint8_t data = uart_read_byte();
            uart_write_byte(data+1);
        }

        system_delay(1000);
    }

    return 0;
}