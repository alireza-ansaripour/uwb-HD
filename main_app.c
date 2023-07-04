
//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(simple_tx);

/* Example application name */
#define APP_NAME "SIMPLE TX v1.0"
#include "instances/instance.h"
int app_main(void){
    LOG_INF("Configuring instance");
    
    LOG_INF("Done config");
    while(1){}
}
