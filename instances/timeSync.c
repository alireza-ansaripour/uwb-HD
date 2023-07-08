#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>
#define TS_FRAME_LEN HDR_LEN + 2


static packet_t timeSync = {
    .type = PACKET_TS,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = 0
};

LOG_MODULE_REGISTER(TIME_SYNC);
void instance_timeSync(){
    
    LOG_INF("Starting TimeSync");
    instance_init();
    dwt_writetxdata(TS_FRAME_LEN, (uint8_t *)&timeSync, 0);
    while(1){
        dwt_writetxfctrl(TS_FRAME_LEN, 0, 0);
        dwt_starttx(DWT_START_TX_IMMEDIATE);

        /* Poll DW IC until TX frame sent event set. See NOTE 4 below.
         * STATUS register is 4 bytes long but, as the event we are looking at
         * is in the first byte of the register, we can use this simplest API
         * function to access it.*/
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
        { /* spin */ };

        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        k_sleep(K_MSEC(100));
        // LOG_INF("TX Frame Sent");

    }
}