#ifndef _INSTANCE_H
#define _INSTANCE_H

#include "deca_types.h"
#include <zephyr.h>
typedef enum{
    PACKET_TS = 0,
    PACKET_DATA
}packet_type;

#define HDR_LEN 11

typedef struct {
    packet_type type;
    uint16_t src;
    uint16_t seq;
    uint16_t dst;
    uint32_t len;
    uint32_t payload[1000]
}packet_t;


void instance_timeSync();
void instance_sender();
void instance_receiver();
void instance_init();
#endif