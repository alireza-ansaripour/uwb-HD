
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
    case  0xf965bd2e:
       instance_timeSync();
       break;


    case 0x2e0bc9bb:
       LOG_INF("Sender 1");
        
       instance_info.addr = 0x0001;
       default_config.txCode = 11;
       instance_info.tx_dly_us = 2100;
       instance_info.dst_addr = 0x0001;
       instance_sender();
       break;

    case 0x920b25f:
       LOG_INF("Sender 2");
       instance_info.addr = 0x0002;
       default_config.txCode = 10;
       instance_info.tx_dly_us = 2100;
       instance_info.dst_addr = 0x0002;
       instance_sender();
    break;

    case 0xc6d8395a:
    
       LOG_INF("Sender 3");
       instance_info.addr = 0x0003;
       default_config.txCode = 9;
       instance_info.tx_dly_us = 2100;
       instance_info.dst_addr = 0x0003;
       instance_sender();
    break;

    case 0x29ec415d:
       LOG_INF("Receiver 1");
       instance_info.addr = 0x0001;
       default_config.rxCode = 11;
       instance_info.tx_dly_us = 2000;
       instance_receiver();
       break;



    case 0xce95d98d:
       LOG_INF("Receiver 2");
       instance_info.addr = 0x0002;
       default_config.rxCode = 10;
       instance_info.tx_dly_us = 2000;
        
       instance_receiver();
       break;
    
    case 0xe8d125b7:
    
       LOG_INF("Receiver 3");
       instance_info.addr = 0x0003;
       default_config.rxCode = 9;
       instance_info.tx_dly_us = 2000;
        
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
