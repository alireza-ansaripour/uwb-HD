#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>

LOG_MODULE_REGISTER(RECEIVER);
packet_t rx_packet;
void instance_receiver(){
    uint32_t status_reg;
    uint16_t frame_len;
    LOG_INF("Starting Receiver");
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
            dwt_readrxdata((uint8_t *)&rx_packet, frame_len, 0); /* No need to read the FCS/CRC. */
            
            LOG_INF("Frame type %d", rx_packet.type);
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