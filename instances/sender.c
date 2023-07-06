#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>
#include "shared_defines.h"
#include <sys/printk.h>
#define TX_FRAME_LEN 100


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

uint32_t rx_time, tx_time, previous_ts;
uint64_t rx_ts_64, prev_ts;
uint64_t rx_time_us;
void instance_sender(){
    uint32_t status_reg;
    uint16_t frame_len;
    prev_ts = 0;
    LOG_INF("Starting Sender");
    instance_init();
    while(1){
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR ))){ /* spin */ };
        if (status_reg & SYS_STATUS_ALL_RX_ERR) {
            if (status_reg & SYS_STATUS_RXPHE_BIT_MASK)  LOG_ERR("receive error: RXPHE");  // Phy. Header Error
            if (status_reg & SYS_STATUS_RXFCE_BIT_MASK)  LOG_ERR("receive error: RXFCE");  // Rcvd Frame & CRC Error
            if (status_reg & SYS_STATUS_RXFSL_BIT_MASK)  LOG_ERR("receive error: RXFSL");  // Frame Sync Loss
            if (status_reg & SYS_STATUS_RXSTO_BIT_MASK)  LOG_ERR("receive error: RXSTO");  // Rcv Timeout
            if (status_reg & SYS_STATUS_ARFE_BIT_MASK)   LOG_ERR("receive error: ARFE");   // Rcv Frame Error
            if (status_reg & SYS_STATUS_CIAERR_BIT_MASK) LOG_ERR("receive error: CIAERR"); //
        }
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

            /* A frame has been received, copy it to our local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
            dwt_readrxdata((uint8_t *)&rx_frame, frame_len, 0); /* No need to read the FCS/CRC. */
            
            LOG_INF("Frame type %d", rx_frame.type);
            switch (rx_frame.type){
            case PACKET_TS:
                rx_time = dwt_readrxtimestamphi32();
                tx_time = (100000 * UUS_TO_DWT_TIME) >> 8;
                tx_time += rx_time;
                dwt_setdelayedtrxtime(tx_time);
                start_TX();
                break;
            
            default:
                break;
            }
            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            // {
            //     char len[5];
            //     sprintf(len, "len %d", frame_len-FCS_LEN);
            //     LOG_HEXDUMP_INF((char*)&rx_buffer, frame_len-FCS_LEN, (char*) &len);
            // }
        }
        else {
            /* Clear RX error events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }

    }
    
        
}


void start_TX(){
    int res;
    tx_frame.seq ++;
    //dwt_forcetrxoff();
    dwt_writetxdata(HDR_LEN + 2, (uint8_t *) &tx_frame, 0);
    dwt_writetxfctrl(TX_FRAME_LEN, 0, 0);
    res = dwt_starttx(DWT_START_TX_DELAYED);
    if (res != DWT_SUCCESS){
      LOG_ERR("TX failed");
    }
    /* Poll DW IC until TX frame sent event set. See NOTE 4 below.
        * STATUS register is 4 bytes long but, as the event we are looking at
        * is in the first byte of the register, we can use this simplest API
        * function to access it.*/
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
    { /* spin */ };

    /* Clear TX frame sent event. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
}