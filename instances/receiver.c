#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include "shared_defines.h"
#include <deca_device_api.h>
#define RX_ANT_DLY 16385

LOG_MODULE_REGISTER(RECEIVER);
packet_t rx_packet;
void instance_receiver(){
    uint8_t rx_type;
    uint32_t status_reg;
    uint16_t frame_len;
    uint32_t rx_time, tx_time;
    int ret;
    LOG_INF("Starting Receiver");
    instance_init();
    dwt_setrxantennadelay(RX_ANT_DLY);
    rx_type = DWT_START_RX_IMMEDIATE;
    ret = dwt_rxenable(rx_type);
    while(1){
        if (ret != DWT_SUCCESS){
            LOG_ERR("FAILED TO START RX");
            LOG_ERR("%ud", tx_time - rx_time);
            rx_type = DWT_START_RX_IMMEDIATE;
            ret = dwt_rxenable(rx_type);
            continue;
        }
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR ))){ /* spin */ };
        if (status_reg & SYS_STATUS_ALL_RX_ERR) {
            if (status_reg & SYS_STATUS_RXPHE_BIT_MASK)  LOG_ERR("receive error: RXPHE");  // Phy. Header Error
            if (status_reg & SYS_STATUS_RXFCE_BIT_MASK)  LOG_ERR("receive error: RXFCE");  // Rcvd Frame & CRC Error
            if (status_reg & SYS_STATUS_RXFSL_BIT_MASK)  LOG_ERR("receive error: RXFSL");  // Frame Sync Loss
            if (status_reg & SYS_STATUS_RXSTO_BIT_MASK)  LOG_ERR("receive error: RXSTO");  // Rcv Timeout
            if (status_reg & SYS_STATUS_ARFE_BIT_MASK)   LOG_ERR("receive error: ARFE");   // Rcv Frame Error
            if (status_reg & SYS_STATUS_CIAERR_BIT_MASK) LOG_ERR("receive error: CIAERR"); //
            rx_type = DWT_START_RX_IMMEDIATE;
            ret = dwt_rxenable(rx_type);
        }
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

            /* A frame has been received, copy it to our local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
            dwt_readrxdata((uint8_t *)&rx_packet, HDR_LEN, 0); /* No need to read the FCS/CRC. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
            //LOG_INF("Frame type %d", rx_packet.type);
            switch(rx_packet.type){
              case PACKET_TS:
                //rx_time = dwt_readsystimestamphi32();
                rx_time = dwt_readrxtimestamphi32();
                tx_time = (instance_info.tx_dly_us * UUS_TO_DWT_TIME) >> 8;
                tx_time += rx_time;
                tx_time &= 0xFFFFFFFEUL;
                dwt_setdelayedtrxtime(tx_time);
                rx_type = DWT_START_RX_DELAYED;
                // rx_type = DWT_START_RX_IMMEDIATE;
                
                ret = dwt_rxenable(rx_type);
                // LOG_INF("TS");
                break;
              case PACKET_DATA:
                rx_type = DWT_START_RX_IMMEDIATE;
                LOG_INF("DATA:DST= %d, SEQ= %d", rx_packet.dst, rx_packet.seq);
                ret = dwt_rxenable(rx_type);
                break;

              
            }
            /* Clear good RX frame event in the DW IC status register. */
            

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