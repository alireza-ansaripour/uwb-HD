#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <sys/printk.h>
#include "shared_defines.h"
#include <math.h>
#include <deca_device_api.h>
#define RX_ANT_DLY 16385



LOG_MODULE_REGISTER(RECEIVER);
packet_t rx_frame;
int i = 0;


#define MY_STACK_SIZE 500
#define MY_PRIORITY 5
K_THREAD_STACK_DEFINE(rx_stack, MY_STACK_SIZE);
struct k_thread t_data;


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
uint16_t rx_frequencies[3000];
uint8_t node_PC;
ts_info_t * rx_ts_info = NULL;


static void start_rx_session(void *a, void *b, void *c){
  //LOG_INF("START RX SESSION");
  dwt_forcetrxoff();
  default_config.rxCode = node_PC;
  instance_radio_config();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  
  k_msleep(rx_ts_info->tx_session_duration - 10);
  dwt_forcetrxoff();
  default_config.rxCode = 9;
  instance_radio_config();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}


void instance_receiver(){
    uint8_t rx_type;
    uint16_t loss = 0;
    uint32_t status_reg;

    uint32_t rx_time, tx_time;
    int ret;
    
    LOG_INF("Starting Receiver");
    instance_init();
    node_PC = default_config.rxCode;
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
    default_config.rxCode = 9;
    instance_radio_config();
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

}


uint32_t packet_cnt = 0;
uint32_t error_cnt = 0;
uint8_t flag = 0;
uint8_t ts_cnt = 0;
uint16_t looking = 1;
uint16_t tx_wait_time[2] = {0, 0};
static void rx_ok_cb(const dwt_cb_data_t *cb_data){
  //dwt_forcetrxoff();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  dwt_readrxdata((uint8_t *)&rx_frame, 17, 0); /* No need to read the FCS/CRC. */
  
  switch (rx_frame.type){
    case PACKET_DATA:
      // LOG_INF("SEQ: %d", rx_frame.seq);
      if (instance_info.addr == rx_frame.dst){
        packet_cnt++;
      }
    break;
    case PACKET_TS:
      LOG_INF("TS %d, %d:err:%d, cnt:%d", tx_wait_time[0], tx_wait_time[1], error_cnt, packet_cnt);
      rx_ts_info = (ts_info_t *) rx_frame.payload;
      if(rx_ts_info->tx_packet_num != 1){
        packet_cnt = 0;
        error_cnt = 0;
      }
      
      ts_cnt++;
      //if (tx_wait_time[0] != rx_ts_info->tx_dly[1] || tx_wait_time[1] != rx_ts_info->tx_dly[2]){
      //  LOG_INF("TS %d, %d:err:%d, cnt:%d", tx_wait_time[0], tx_wait_time[1], error_cnt, packet_cnt);
      //  error_cnt = 0;
      //  packet_cnt = 0;
      //}

      //tx_wait_time[0] = rx_ts_info->tx_dly[1];
      //tx_wait_time[1] = rx_ts_info->tx_dly[2];
      //dwt_forcetrxoff();
      //default_config.rxCode = node_PC;
      //instance_radio_config();
      //dwt_rxenable(DWT_START_RX_IMMEDIATE);      
      
      k_thread_create(&t_data, rx_stack,
                                 K_THREAD_STACK_SIZEOF(rx_stack),
                                 start_rx_session,
                                 NULL, NULL, NULL,
                                 MY_PRIORITY, 0, K_NO_WAIT);
      
      

    break;
  }
}
static void rx_err_cb(const dwt_cb_data_t *cb_data){
  dwt_forcetrxoff();
  error_cnt ++;
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}