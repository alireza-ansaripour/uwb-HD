#include "instance.h"
#include <zephyr/logging/log.h>
#include "deca_regs.h"
#include <deca_device_api.h>



static packet_t timeSync = {
    .type = PACKET_TS,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = 0
};

static packet_t timeSync_ack = {
    .type = PACKET_TS_ACK,
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
    int res = 0;
    int ts_num = 0;
    LOG_INF("Starting TimeSync");
    instance_init();
    
    // ts_duration_ms = 12000;
    // tx_packet_num  = 10000;
    int tx_count = 0;

    ts_duration_ms = 90;
    tx_packet_num  = 50;


    ts_info_t *ts_info = (ts_info_t *)timeSync.payload;

    
    ts_info->tx_session_duration = ts_duration_ms;
    
    
    
    while(1){
        tx_count++;
        ts_info->tx_dly[1] = 1600;
        ts_info->tx_dly[2] = 1600;
        
        ts_info->tx_packet_num = tx_packet_num;

        gpio_set(PORT_DE);
        timeSync.type = PACKET_TS;
        timeSync.seq++;
        dwt_writetxdata(TS_FRAME_LEN, (uint8_t *)&timeSync, 0);
        dwt_writetxfctrl(TS_FRAME_LEN, 0, 0);
        dwt_starttx(DWT_START_TX_IMMEDIATE);

        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
        { /* spin */ };

        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        gpio_reset(PORT_DE);
        ts_num++;
        k_sleep(K_MSEC(ts_duration_ms));

        ///////////////////////////////////////////////////////////

        timeSync_ack.seq++;
        dwt_writetxdata(TS_FRAME_LEN, (uint8_t *)&timeSync_ack, 0);
        dwt_writetxfctrl(TS_FRAME_LEN, 0, 0);
        res = dwt_starttx(DWT_START_TX_IMMEDIATE);

        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
        { /* spin */ };

        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        gpio_reset(PORT_DE);
        ts_num++;
        LOG_INF("TS NUM %d", timeSync_ack.seq);
        k_sleep(K_MSEC(5));
        

    }
    while(1){}
}