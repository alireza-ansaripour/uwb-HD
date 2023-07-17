#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <sys/printk.h>
#include "shared_defines.h"
#include <deca_device_api.h>
#define RX_ANT_DLY 16385



LOG_MODULE_REGISTER(RECEIVER);
packet_t rx_packet;
int i = 0;

#define ANIMATE_PING_STACK    1024
#define QUEUE_SIZE            10100
#define ITEM_LEN              4

// K_MSGQ_DEFINE(rx_seq_msq,
//         ITEM_LEN,
//         QUEUE_SIZE,
//         4);



// /* All animations handled by this thread */
// extern void animate_ping_thread(void *d0, void *d1, void *d2) {
//     uint16_t rx_seq;
//     while(1) {
//         // gpio_set(PORT_DE);
//         k_sleep(K_SECONDS(1));
//         // gpio_reset(PORT_DE);
//         // if (i < 1000)
//         //     continue;
//         // k_msgq_get(&rx_seq_msq, &rx_seq, K_FOREVER);
//         // printk("%ud\n", rx_seq);
//     }
// }

// K_THREAD_DEFINE(animate_ping_tid, ANIMATE_PING_STACK,
//             animate_ping_thread, NULL, NULL, NULL,
//             K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
uint16_t rx_seq = 1;
uint32_t ts1 = 0, ts2 = 0;
uint16_t first_seq_num = 0xffff;
uint16_t frame_len, ip_diag_12;
void instance_receiver(){
    uint8_t rx_type;
    uint16_t loss = 0;
    uint32_t status_reg;

    uint32_t rx_time, tx_time;
    int ret;
    
    LOG_INF("Starting Receiver");
    instance_init();
    // k_thread_start(animate_ping_tid);
    
    
    
    dwt_setrxantennadelay(RX_ANT_DLY);
    rx_type = DWT_START_RX_IMMEDIATE;
    ret = dwt_rxenable(rx_type);
    if (ret != DWT_SUCCESS){
        LOG_ERR("FAILED TO START RX");
        LOG_ERR("%ud", tx_time - rx_time);
        rx_type = DWT_START_RX_IMMEDIATE;
        ret = dwt_rxenable(rx_type);
    }


    while(1){

        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR ))){ /* spin */ };
        if (status_reg & SYS_STATUS_ALL_RX_ERR) {
            // if (status_reg & SYS_STATUS_RXPHE_BIT_MASK)  LOG_ERR("receive error: RXPHE");  // Phy. Header Error
            // if (status_reg & SYS_STATUS_RXFCE_BIT_MASK)  LOG_ERR("receive error: RXFCE");  // Rcvd Frame & CRC Error
            // if (status_reg & SYS_STATUS_RXFSL_BIT_MASK)  LOG_ERR("receive error: RXFSL");  // Frame Sync Loss
            // if (status_reg & SYS_STATUS_RXSTO_BIT_MASK)  LOG_ERR("receive error: RXSTO");  // Rcv Timeout
            // if (status_reg & SYS_STATUS_ARFE_BIT_MASK)   LOG_ERR("receive error: ARFE");   // Rcv Frame Error
            // if (status_reg & SYS_STATUS_CIAERR_BIT_MASK) LOG_ERR("receive error: CIAERR"); //
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
                // i++;
                rx_time = dwt_readrxtimestamphi32();
                tx_time = (instance_info.tx_dly_us * UUS_TO_DWT_TIME) >> 8;
                tx_time += rx_time;
                tx_time &= 0xFFFFFFFEUL;
                dwt_setdelayedtrxtime(tx_time);
                rx_type = DWT_START_RX_DELAYED;
                // rx_type = DWT_START_RX_IMMEDIATE;
                ret = dwt_rxenable(rx_type);
                if (ret != DWT_SUCCESS){

                    LOG_ERR("RX failed");
                }
                LOG_INF("%ud, %ud", rx_seq, first_seq_num);
                first_seq_num = 0xffff;
                rx_seq = 0;
                ts1 = 0;
                ts2 = 0;
                break;
              case PACKET_DATA:
                rx_type = DWT_START_RX_IMMEDIATE;
                ret = dwt_rxenable(rx_type);
                if (ret != DWT_SUCCESS){

                    LOG_ERR("RX failed");
                }




                if (rx_packet.dst != instance_info.addr)
                     continue;
                if (first_seq_num == 0xffff)
                    first_seq_num = rx_packet.seq;
                 rx_seq++;

                break;
                default:
                    rx_type = DWT_START_RX_IMMEDIATE;
                    ret = dwt_rxenable(rx_type);
                break;
            }
        }
        else {
            /* Clear RX error events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
            rx_type = DWT_START_RX_IMMEDIATE;
            ret = dwt_rxenable(rx_type);
        }

    }
    
        
}