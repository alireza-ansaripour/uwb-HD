#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>


static packet_t timeSync = {
    .type = PACKET_TS,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = 0
};

LOG_MODULE_REGISTER(RECEIVER);
void instance_receiver(){
    
    LOG_INF("Starting Receiver");
    instance_init();
    
}