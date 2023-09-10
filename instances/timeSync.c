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

// static ts_info_t ts_info;
#define TS_PER_ROUND          100
LOG_MODULE_REGISTER(TIME_SYNC);
uint16_t ts_duration_ms;
uint16_t tx_packet_num;

uint16_t ts[11][2] = {{1200, 1200}, {1201, 1200}, {1202, 1200}, {1203, 1200}, {1204, 1200}, {1205, 1200}, {1206, 1200}, {1207, 1200}, {1208, 1200}, {1209, 1200}, {1210, 1200}, {1211, 1200}, {1212, 1200}, {1213, 1200}, {1214, 1200}, {1215, 1200}};


void instance_timeSync(){
    int cnt = 0;
    int ts_num = 0;
    LOG_INF("Starting TimeSync");
    instance_init();
    
    // ts_duration_ms = 12000;
    // tx_packet_num  = 10000;
    
    ts_duration_ms = 500;
    tx_packet_num  = 1;


    ts_info_t *ts_info = (ts_info_t *)timeSync.payload;

    ts_info->tx_packet_num = tx_packet_num;
    ts_info->tx_session_duration = ts_duration_ms;
    

    
    while(1){
        cnt = (ts_num / TS_PER_ROUND) % (sizeof(ts) / 4); 
        ts_info->tx_dly[1] = ts[cnt][1];
        ts_info->tx_dly[2] = ts[cnt][0];    
        gpio_set(PORT_DE);
        dwt_writetxdata(TS_FRAME_LEN, (uint8_t *)&timeSync, 0);
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
        if (ts_num % TS_PER_ROUND == 0){
          k_sleep(K_MSEC(1000));
        }
        ts_num++;
        k_sleep(K_MSEC(ts_duration_ms));
        // LOG_INF("TS Frame Sent");

    }
}