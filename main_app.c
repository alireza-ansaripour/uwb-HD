
//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
#include "dk_buttons_and_leds.h"
LOG_MODULE_REGISTER(simple_tx);

/* Example application name */
#define APP_NAME "SIMPLE TX v1.0"
#include "instances/instance.h"
int app_main(void){
    uint32_t dev_id = NRF_FICR->DEVICEADDR[0];
    switch (dev_id)
    {
    case  0x2e0bc9bb:
        instance_timeSync();
        break;
    case  0xf965bd2e:
        instance_info.addr = 0x0001;
        instance_info.tx_dly_us = 100000;
        instance_sender();
        break;
    case 0x29ec415d:
        instance_info.addr = 0x0002;
        instance_info.tx_dly_us = 10000;
        instance_receiver();
        break;
    default:
        LOG_INF("DEV_ID: 0x%x", dev_id);
        break;
    }

    

    LOG_INF("Configuring instance");
    
    LOG_INF("Done config");
    while(1){}
}
