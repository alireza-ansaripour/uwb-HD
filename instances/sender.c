#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>
#include "shared_defines.h"
#include "shared_functions.h"
#include <sys/printk.h>
#define TX_FRAME_LEN 50
#define TX_ANT_DLY 16385

static packet_t tx_frame = {
    .type = PACKET_DATA,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = TX_FRAME_LEN
};

uint8_t msg[] = {1,2,3,4,5,6};

static packet_t rx_frame;

LOG_MODULE_REGISTER(SENDER);
void start_TX();
int transmit(uint8_t type);

uint32_t rx_time, previous_ts;
uint64_t tx_time;
uint64_t rx_ts_64, prev_ts;
uint64_t rx_time_us;


uint64_t rx_ts_64, tx_ts_64, prev;
uint32_t final_tx_time;
void instance_sender(){
    uint32_t status_reg;
    uint16_t frame_len;
    prev_ts = 0;
    LOG_INF("Starting Sender");
    tx_frame.src = instance_info.addr;
    tx_frame.dst = instance_info.dst_addr;
    instance_init();
    dwt_settxantennadelay(TX_ANT_DLY);
    while(1){
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR ))){ /* spin */ };
        if (status_reg & SYS_STATUS_ALL_RX_ERR) {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
            if (status_reg & SYS_STATUS_RXPHE_BIT_MASK)  LOG_ERR("receive error: RXPHE");  // Phy. Header Error
            if (status_reg & SYS_STATUS_RXFCE_BIT_MASK)  LOG_ERR("receive error: RXFCE");  // Rcvd Frame & CRC Error
            if (status_reg & SYS_STATUS_RXFSL_BIT_MASK)  LOG_ERR("receive error: RXFSL");  // Frame Sync Loss
            if (status_reg & SYS_STATUS_RXSTO_BIT_MASK)  LOG_ERR("receive error: RXSTO");  // Rcv Timeout
            if (status_reg & SYS_STATUS_ARFE_BIT_MASK)   LOG_ERR("receive error: ARFE");   // Rcv Frame Error
            if (status_reg & SYS_STATUS_CIAERR_BIT_MASK) LOG_ERR("receive error: CIAERR"); //
        }
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
            dwt_forcetrxoff();
            /* A frame has been received, copy it to our local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
            dwt_readrxdata((uint8_t *)&rx_frame, HDR_LEN, 0); /* No need to read the FCS/CRC. */
            
            
            switch (rx_frame.type){
            case PACKET_TS:
                rx_ts_64 = get_rx_timestamp_u64();
                final_tx_time = (rx_ts_64 + (instance_info.tx_dly_us * UUS_TO_DWT_TIME)) >> 8;
                dwt_setdelayedtrxtime(final_tx_time);
                start_TX();
                // LOG_INF("TS MSG %d", tx_frame.seq);
                // LOG_INF("__________________");
                prev = rx_ts_64;
                break;
            
            default:
                LOG_INF("RX frame len: %d, type %d", frame_len, rx_frame.type);
                break;
            }
            // /* Clear good RX frame event in the DW IC status register. */
            

            // // {
            // //     char len[5];
            // //     sprintf(len, "len %d", frame_len-FCS_LEN);
            // //     LOG_HEXDUMP_INF((char*)&rx_buffer, frame_len-FCS_LEN, (char*) &len);
            // // }
        }

    }
    
        
}

uint32_t curr_time;


void start_TX(){
    int res;
    tx_frame.seq ++;
    res = transmit(DWT_START_TX_DELAYED);
    if (res != DWT_SUCCESS){
        LOG_ERR("TX failed");
        return;
    }
    for (uint8_t i = 0; i < 30; i++){
        tx_frame.seq ++;
        res = transmit(DWT_START_TX_IMMEDIATE);
        if (res != DWT_SUCCESS){
            LOG_ERR("TX failed %ud", i);
            return;
        }
    }
    
}


int transmit(uint8_t type){
    int res;
    dwt_writetxfctrl(TX_FRAME_LEN, 0, 0);
    dwt_writetxdata(HDR_LEN + 2, (uint8_t *) &tx_frame, 0);
    res = dwt_starttx(type);
    if (res != DWT_SUCCESS){
      return res;
    }
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){ /* spin */ };
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    dwt_forcetrxoff();
    return DWT_SUCCESS;
}
