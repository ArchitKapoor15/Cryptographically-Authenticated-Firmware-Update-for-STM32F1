#include <string.h>
#include "comms.h"
#include "core/uart.h"
#include "core/crc8.h"

#define PKT_BUFFER_SIZE 8

typedef enum comms_state_t{
    state_length,
    state_data,
    state_crc,
}comms_state_t;

comms_state_t state = state_length;
static uint8_t data_counter = 0;

static packet_t temp_pkt = {0U};
static packet_t retx_pkt = {0U};
static packet_t ack_pkt = {0U};
static packet_t last_pkt = {0U};

static packet_t pkt_buffer[PKT_BUFFER_SIZE];
static uint32_t pkt_read_idx = 0;
static uint32_t pkt_write_idx = 0;
static uint32_t pkt_mask = PKT_BUFFER_SIZE-1;

bool is_sb_pkt(const packet_t* pkt, uint8_t byte){
    if(pkt->length != 1){
        return false;
    }
    if(pkt->data[0] != byte){
        return false;
    }
    for(uint8_t i=1;i<PACKET_DATA_LENGTH;i++){
        if(pkt->data[i]!=0xFF){
            return false;
        }
    }
    return true;
}

void create_sb_pkt(packet_t* pkt, uint8_t byte){
    memset(pkt,0xff,sizeof(packet_t));
    pkt->length = 1;
    pkt->data[0] = byte;
    pkt->CRC = comms_compute_crc(pkt);
}

void comms_setup(void){
    create_sb_pkt(&retx_pkt,RETX_DATA);
    create_sb_pkt(&ack_pkt,ACK_DATA);
}

void comms_update(void){
    while(uart_data_available()){
        switch(state){
            case state_length : {
                temp_pkt.length = uart_read_byte();
                state = state_data;
            }break;

            case state_data : {
                temp_pkt.data[data_counter++] = uart_read_byte();
                if(data_counter>=PACKET_DATA_LENGTH){
                    data_counter = 0;
                    state = state_crc;
                }
            }break;

            case state_crc : {
                temp_pkt.CRC = uart_read_byte();
                if(temp_pkt.CRC != comms_compute_crc(&temp_pkt)){
                    comms_send(&retx_pkt);
                    state=state_length;
                    break;
                }
                if(is_sb_pkt(&temp_pkt,RETX_DATA)){
                    comms_send(&last_pkt);
                    state=state_length;
                    break;
                }
                if(is_sb_pkt(&temp_pkt,ACK_DATA)){
                    state=state_length;
                    break;
                }

                uint32_t nxt_write_idx = (pkt_write_idx+1)&pkt_mask;
                if(nxt_write_idx==pkt_read_idx){
                    __asm__("BKPT #0");
                }
                memcpy(&pkt_buffer[pkt_write_idx],&temp_pkt,sizeof(packet_t));
                pkt_write_idx = nxt_write_idx;
                comms_send(&ack_pkt);
                state = state_length;

            }break;

            default : {
                state = state_length;
            }
        }
    }
}

bool comms_pkt_available(void){
    return pkt_read_idx!=pkt_write_idx;
}

void comms_send(packet_t* pkt){
    uart_write((uint8_t*)pkt,PACKET_DATA_LENGTH+2);
    memcpy(&last_pkt,pkt,sizeof(packet_t));
}

void comms_read(packet_t* pkt){
    memcpy(pkt,&pkt_buffer[pkt_read_idx],sizeof(packet_t));
    pkt_read_idx = (pkt_read_idx+1)&pkt_mask;
}

uint8_t comms_compute_crc(packet_t* pkt){
    return crc((uint8_t*)pkt,PACKET_DATA_LENGTH + 1);
}
