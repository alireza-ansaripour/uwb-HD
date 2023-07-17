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

extern dwt_config_t default_config;

typedef struct{
  uint16_t addr;
  uint16_t dst_addr;
  uint32_t tx_dly_us;
}instance_info_t; 

#define LED_RX GPIO_DIR_GDP2_BIT_MASK
#define LED_TX GPIO_DIR_GDP3_BIT_MASK
#define PORT_DE GPIO_DIR_GDP4_BIT_MASK

void gpio_set(uint16_t port);
void gpio_reset(uint16_t port);
void instance_timeSync();
void instance_sender();
void instance_receiver();
void instance_init();
extern instance_info_t instance_info; 
#endif