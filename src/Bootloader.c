#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/vector.h>

#include "core/uart.h"
#include "core/system.h"
#include "comms.h"
#include "bl-flash.h"
#include "core/simple-timer.h"

#define UART_PORT GPIOA
#define TX_PIN GPIO9
#define RX_PIN GPIO10

#define BOOTLOADER_SIZE (0x2000U)
#define MAIN_APP_START_ADD (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FLASH_APP_SIZE ((1024U * 128U) - BOOTLOADER_SIZE)

#define DEVICE_ID (0x42)

#define SYNC_SEQ_0 (0xC4)
#define SYNC_SEQ_1 (0x55)
#define SYNC_SEQ_2 (0x7E)
#define SYNC_SEQ_3 (0x10)

#define DEFAULT_TIMEOUT (5000)

typedef enum bl_state_t{
    State_Sync,
    State_WaitForUpdateReq,
    State_DeviceIDReq,
    State_DeviceIDRes,
    State_FWLenReq,
    State_FWLenRes,
    State_EraseApp,
    State_ReceiveFW,
    State_Done,
}bl_state_t;

static bl_state_t state = State_Sync;
static uint32_t fw_len = 0;
static uint32_t bytes_written = 0;
static uint8_t sync_seq[4] = {0};
static timer_t timer;
static packet_t packet;

static void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(UART_PORT,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,TX_PIN);
    gpio_set_mode(UART_PORT,GPIO_MODE_INPUT,GPIO_CNF_INPUT_FLOAT,RX_PIN);
}

static void gpio_teardown(void){
    gpio_set_mode(UART_PORT,GPIO_MODE_INPUT,GPIO_CNF_INPUT_FLOAT,TX_PIN);
    gpio_set_mode(UART_PORT,GPIO_MODE_INPUT,GPIO_CNF_INPUT_FLOAT,RX_PIN);
    rcc_periph_clock_disable(RCC_GPIOA);
}

static void jump_to_main(void){
    vector_table_t* main_vector_table = (vector_table_t*)MAIN_APP_START_ADD;
    main_vector_table->reset();
}

static void bootloading_failed(void){
    create_sb_pkt(&packet,BL_NACK);
    comms_send(&packet);
    state = State_Done;
}

static void check_for_timeout(void){
    if(simple_timer_has_elapsed(&timer)){
        bootloading_failed();
    }
}

static bool is_device_id_pkt(const packet_t* pkt){
    if(pkt->length != 2){
        return false;
    }

    if(pkt->data[0] != BL_DEV_ID_RES){
        return false;
    }

    for(uint8_t i=2; i<PACKET_DATA_LENGTH; i++){
        if(pkt->data[i] != 0xFF){
            return false;
        }
    }

    return true;
}

static bool is_fw_len_pkt(const packet_t* pkt){
    if(pkt->length != 5){
        return false;
    }

    if(pkt->data[0] != BL_FW_LEN_RES){
        return false;
    }

    for(uint8_t i=5; i<PACKET_DATA_LENGTH; i++){
        if(pkt->data[i] != 0xFF){
            return false;
        }
    }
    
    return true;
}

int main(void){
    system_setup();
    uart_setup();
    gpio_setup();
    comms_setup();

    simple_timer_setup(&timer,DEFAULT_TIMEOUT,false);

    while(state != State_Done){
        if(state == State_Sync){
            if(uart_data_available()){
                sync_seq[0] = sync_seq[1];
                sync_seq[1] = sync_seq[2];
                sync_seq[2] = sync_seq[3];
                sync_seq[3] = uart_read_byte();

                bool is_match = sync_seq[0] == SYNC_SEQ_0;
                is_match = is_match && (sync_seq[1] == SYNC_SEQ_1);
                is_match = is_match && (sync_seq[2] == SYNC_SEQ_2);
                is_match = is_match && (sync_seq[3] == SYNC_SEQ_3);

                if(is_match){
                    create_sb_pkt(&packet,BL_SYNC_OBS);
                    comms_send(&packet);
                    simple_timer_reset(&timer);
                    state = State_WaitForUpdateReq;
                }else{
                    check_for_timeout();
                }
            }else{
                check_for_timeout();
            }
            continue;
        }

        comms_update();

        switch(state){
            case State_WaitForUpdateReq : {
                if(comms_pkt_available()){
                    comms_read(&packet);
                    if(is_sb_pkt(&packet,BL_FW_UPD_REQ)){
                        simple_timer_reset(&timer);
                        create_sb_pkt(&packet,BL_FW_UPD_RES);
                        comms_send(&packet);
                        state = State_DeviceIDReq;
                    }else{
                        bootloading_failed();
                    }
                }else{
                    check_for_timeout();
                }
            } break;

            case State_DeviceIDReq : {
                simple_timer_reset(&timer);
                create_sb_pkt(&packet,BL_DEV_ID_REQ);
                comms_send(&packet);
                state = State_DeviceIDRes;
            } break;

            case State_DeviceIDRes : {
                if(comms_pkt_available()){
                    comms_read(&packet);
                    if(is_device_id_pkt(&packet) && (packet.data[1] == DEVICE_ID)){
                        simple_timer_reset(&timer);
                        state = State_FWLenReq;
                    }else{
                        bootloading_failed();
                    }
                }else{
                    check_for_timeout();
                }
            } break;

            case State_FWLenReq : {
                simple_timer_reset(&timer);
                create_sb_pkt(&packet,BL_FW_LEN_REQ);
                comms_send(&packet);
                state = State_FWLenRes;
            } break;

            case State_FWLenRes : {
                if(comms_pkt_available()){
                    comms_read(&packet);

                    fw_len = (
                        (packet.data[1])       |
                        (packet.data[2] << 8)  |
                        (packet.data[3] << 16) |
                        (packet.data[4] << 24)
                    );

                    if(is_fw_len_pkt(&packet) && (fw_len <= MAX_FLASH_APP_SIZE)){
                        state = State_EraseApp;
                    }else{
                        bootloading_failed();
                    }
                }else{
                    check_for_timeout();
                }
            } break;

            case State_EraseApp : {
                bl_flash_erase_app();
                create_sb_pkt(&packet,BL_READY_FOR_DATA);
                comms_send(&packet);
                simple_timer_reset(&timer);
                state = State_ReceiveFW;
            } break;

            case State_ReceiveFW : {
                if(comms_pkt_available()){
                    comms_read(&packet);
                    const uint8_t pkt_len = (packet.length & 0x0F)+1;
                    bl_flash_write(MAIN_APP_START_ADD+bytes_written,packet.data,pkt_len);
                    bytes_written += pkt_len;
                    simple_timer_reset(&timer);
                    if(bytes_written >= fw_len){
                        create_sb_pkt(&packet,BL_FW_UPDATE_SUCCESSFUL);
                        comms_send(&packet);
                        state = State_Done;
                    }else{
                        create_sb_pkt(&packet,BL_READY_FOR_DATA);
                        comms_send(&packet);
                    }
                }else{
                    check_for_timeout();
                }
            } break;
            
            default : {
                state = State_Sync;
            }
        }
    }

    system_delay(150);
    gpio_teardown();
    uart_teardown();
    system_teardown();

    jump_to_main();

    return 0;
}