#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <sys/printk.h>
#include "shared_defines.h"
#include <math.h>
#include <deca_device_api.h>
#define RX_ANT_DLY 16385



LOG_MODULE_REGISTER(RECEIVER);
packet_t rx_packet;
int i = 0;

#define ANIMATE_PING_STACK    1024
#define QUEUE_SIZE            10100
#define ITEM_LEN              4

#define COUNT                 1400
// K_MSGQ_DEFINE(rx_seq_msq,
//         ITEM_LEN,
//         QUEUE_SIZE,
//         4);



static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);
            
uint16_t rx_seq = 1;
uint32_t ts1 = 0, ts2 = 0;
uint16_t first_seq_num = 0xffff;
uint16_t frame_len, ip_diag_12;
uint16_t rx_frequencies[1500];

void instance_receiver(){
    uint8_t rx_type;
    uint16_t loss = 0;
    uint32_t status_reg;

    uint32_t rx_time, tx_time;
    int ret;
    
    LOG_INF("Starting Receiver");
    instance_init();

    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_setcallbacks(NULL, &rx_ok_cb,NULL,&rx_err_cb, NULL, NULL);
    dwt_setinterrupt(SYS_ENABLE_LO_TXFRS_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXFCG_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXFTO_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXPTO_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXPHE_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXFCE_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXFSL_ENABLE_BIT_MASK |
                    SYS_ENABLE_LO_RXSTO_ENABLE_BIT_MASK,
                    0,
    DWT_ENABLE_INT);

    /* Clearing the SPI ready interrupt */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RCINIT_BIT_MASK | SYS_STATUS_SPIRDY_BIT_MASK);

    // /* Install DW IC IRQ handler. */
    port_set_dwic_isr(dwt_isr);

}


static void rx_ok_cb(const dwt_cb_data_t *cb_data){
  
}
static void rx_err_cb(const dwt_cb_data_t *cb_data){
  dwt_forcetrxoff();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}