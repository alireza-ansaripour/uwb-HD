#include "instance.h"
#include <zephyr/logging/log.h>
#include "deca_regs.h"
#include "shared_defines.h"
#include <math.h>
#include <deca_device_api.h>
#define RX_ANT_DLY 16385



LOG_MODULE_REGISTER(RECEIVER);
packet_t rx_frame;
int rx_timestamp;
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
static void tx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);
            
uint16_t rx_seq = 1;
uint32_t ts1 = 0, ts2 = 0;
uint16_t first_seq_num = 0xffff;
uint16_t frame_len, ip_diag_12;
uint16_t rx_frequencies[3000];
uint8_t node_PC;
uint32_t packet_cnt = 0;
uint32_t error_cnt = 0;

uint8_t ts_cnt = 0;
uint8_t ts_ack_cnt = 0;
uint16_t looking = 1;
uint16_t tx_wait_time[2] = {0, 0};


ts_info_t * rx_ts_info = NULL;
packet_t ack_frame;


static void start_rx_session(void *a, void *b, void *c){
  //LOG_INF("START RX SESSION");
  dwt_forcetrxoff();
  default_config.rxCode = node_PC;
  instance_radio_config();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  k_msleep(rx_ts_info->tx_session_duration - 5);
  dwt_forcetrxoff();
  default_config.rxCode = 9;
  instance_radio_config();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}



static void send_ack_msg(void *a, void *b, void *c){
  //LOG_INF("START RX SESSION");
  int res;
  dwt_forcetrxoff();
  // default_config.txCode = node_PC;
  // instance_radio_config();
  // k_msleep(2);
  dwt_writetxfctrl(40, 0, 0);
  ack_frame.type = PACKET_ACK;
  ack_frame.dst = instance_info.addr;
  ack_info_t * ack_payload = (ack_info_t *) ack_frame.payload;
  ack_payload->pkt_recv_cnt = looking;
  ack_payload->error_cnt = error_cnt;
  dwt_writetxdata(40, (uint8_t *) &ack_frame, 0);
  // dwt_setdelayedtrxtime(dwt_readrxtimestamphi32() + (uint32_t) ((850 * UUS_TO_DWT_TIME) >> 8) );
  dwt_setdelayedtrxtime(dwt_readrxtimestamphi32() + (uint32_t) ((1000 * UUS_TO_DWT_TIME) >> 8) );
  res = dwt_starttx(DWT_START_TX_DELAYED);
  if (res != DWT_SUCCESS){
    LOG_ERR("TX alireza failed");
    dwt_forcetrxoff();
  }




  k_msleep(5);
  dwt_forcetrxoff();
  default_config.rxCode = 9;
  instance_radio_config();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  LOG_INF("TS:err:%d, cnt:%d\n",error_cnt, packet_cnt);
  packet_cnt = 0;
  error_cnt = 0;



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
    dwt_setcallbacks(&tx_ok_cb, &rx_ok_cb,NULL,&rx_err_cb, NULL, NULL);
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
    default_config.txCode = node_PC;
    instance_radio_config();
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

}


int loss_cnt_thresh = 10;
uint16_t loss_cnt;
int loss_diff;
int rx_cnt = 0; 
static void rx_ok_cb(const dwt_cb_data_t *cb_data){
  rx_timestamp = 0;
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  dwt_readrxdata((uint8_t *)&rx_frame, 17, 0); /* No need to read the FCS/CRC. */
  switch (rx_frame.type){

  case PACKET_TS_ACK:
    dwt_forcetrxoff();
    k_thread_create(&t_data, rx_stack,
                K_THREAD_STACK_SIZEOF(rx_stack),
                send_ack_msg,
                NULL, NULL, NULL,
                MY_PRIORITY, 0, K_NO_WAIT);
    
    LOG_INF("ACK INFO: %d, %d", rx_cnt, looking);
    break;

  case PACKET_TS:
    loss_cnt = 0;
    rx_cnt = 0;
    rx_ts_info = (ts_info_t *) rx_frame.payload;
    ts_cnt++;
    k_thread_create(&t_data, rx_stack,
                    K_THREAD_STACK_SIZEOF(rx_stack),
                    start_rx_session,
                    NULL, NULL, NULL,
                    MY_PRIORITY, 0, K_NO_WAIT);
    break;
  case PACKET_DATA:
    rx_cnt ++;
    // LOG_INF("looking %d, SEQ: %d", looking, rx_frame.seq);
    if (looking > rx_frame.seq)
      return;
    if(looking == rx_frame.seq){
      looking++;
    }else{
      loss_diff = rx_frame.seq - looking;
      if ((loss_diff + loss_cnt) > loss_cnt_thresh){
        // 
      }else{
        loss_cnt += loss_diff;
        looking = rx_frame.seq + 1;
      }  
    }
    break;
  default:
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    break;
  }




}
static void rx_err_cb(const dwt_cb_data_t *cb_data){
  // LOG_INF("EEEEEEEE");
  dwt_forcetrxoff();
  error_cnt ++;
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

static void tx_ok_cb(const dwt_cb_data_t *cb_data){
  dwt_forcetrxoff();
  //error_cnt ++;
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}