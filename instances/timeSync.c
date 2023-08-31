#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>
#define TS_FRAME_LEN 16 + 2


static packet_t timeSync = {
    .type = PACKET_TS,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = 0
};

// static ts_info_t ts_info;

LOG_MODULE_REGISTER(TIME_SYNC);
uint16_t ts_duration_ms;
uint16_t tx_packet_num;
void instance_timeSync(){
    
    LOG_INF("Starting TimeSync");
    instance_init();
    
    // ts_duration_ms = 12000;
    // tx_packet_num  = 10000;
    
    ts_duration_ms = 100;
    tx_packet_num  = 1;


    ts_info_t *ts_info = (ts_info_t *)timeSync.payload;

    ts_info->tx_packet_num = tx_packet_num;
    ts_info->tx_session_duration = ts_duration_ms;

    dwt_writetxdata(TS_FRAME_LEN, (uint8_t *)&timeSync, 0);
    while(1){
        gpio_set(PORT_DE);
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
        gpio_reset(PORT_DE);

        k_sleep(K_MSEC(ts_duration_ms));
        // LOG_INF("TS Frame Sent");

    }
}