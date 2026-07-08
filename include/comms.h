#ifndef INC_COMMS_H
#define INC_COMMS_H

#include "common-defines.h"

#define PACKET_DATA_LENGTH (16)
#define RETX_DATA          (0x19)
#define ACK_DATA           (0x15)

#define BL_SYNC_OBS             (0X20)
#define BL_FW_UPD_REQ           (0x31)
#define BL_FW_UPD_RES           (0x37)
#define BL_DEV_ID_REQ           (0x3C)
#define BL_DEV_ID_RES           (0x3F)
#define BL_FW_LEN_REQ           (0x42)
#define BL_FW_LEN_RES           (0x45)
#define BL_READY_FOR_DATA       (0x48)
#define BL_FW_UPDATE_SUCCESSFUL (0x54)
#define BL_NACK                 (0x59)

typedef struct packet_t{
    uint8_t length;
    uint8_t data[PACKET_DATA_LENGTH];
    uint8_t CRC;
}packet_t;

void comms_setup(void);
void comms_update(void);
bool comms_pkt_available(void);
void comms_send(packet_t* pkt);
void comms_read(packet_t* pkt);
uint8_t comms_compute_crc(packet_t* pkt);
bool is_sb_pkt(const packet_t* pkt, uint8_t byte);
void create_sb_pkt(packet_t* pkt, uint8_t byte);

#endif