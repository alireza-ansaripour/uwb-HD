#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>
#include "shared_defines.h"
#include "shared_functions.h"
#include <sys/printk.h>
#define TX_FRAME_LEN 1000
#define TX_ANT_DLY 16385


// #define CONT_FRAME_PERIOD 420000
// #define TX_PER_ROUND      1150
#define CONT_FRAME_PERIOD 420000
#define TX_PER_ROUND      1000

/* Continuous frame duration, in milliseconds. */
#define CONT_FRAME_DURATION_MS 120000

static packet_t tx_frame = {
    .type = PACKET_DATA,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = TX_FRAME_LEN
};

static uint8_t tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};


static packet_t rx_frame;

LOG_MODULE_REGISTER(SENDER);

void start_TX();
int transmit(uint8_t type);


/* Declaration of static functions. */
static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_to_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);
static void tx_conf_cb(const dwt_cb_data_t *cb_data);

uint32_t rx_time, previous_ts;
uint64_t tx_time;
uint64_t rx_ts_64, prev_ts;
uint64_t rx_time_us;


uint64_t rx_ts_64, tx_ts_64, prev;
uint32_t final_tx_time;

void costume_isr(){
   
    dwt_isr();
}

void instance_sender(){
    uint32_t status_reg;
    uint16_t frame_len;
    prev_ts = 0;
    LOG_INF("Starting Sender");
    tx_frame.src = instance_info.addr;
    tx_frame.dst = instance_info.dst_addr;
    instance_init();
    gpio_set(PORT_DE);
    dwt_settxantennadelay(TX_ANT_DLY);
    dwt_setcallbacks(&tx_conf_cb, &rx_ok_cb,NULL,NULL, NULL, NULL);
    dwt_setinterrupt(
        SYS_ENABLE_LO_TXFRS_ENABLE_BIT_MASK|
        SYS_ENABLE_LO_RXFCG_ENABLE_BIT_MASK,
        0,
        DWT_ENABLE_INT);

    /* Clearing the SPI ready interrupt */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RCINIT_BIT_MASK | SYS_STATUS_SPIRDY_BIT_MASK);

    // /* Install DW IC IRQ handler. */
    port_set_dwic_isr(dwt_isr);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    while(1){
    }
    
        
}
int c = 1, GXX=0;

uint64_t tx_timestamp;
int res;
static void tx_conf_cb(const dwt_cb_data_t *cb_data){
    tx_frame.seq++;
    if(c==1){
      dwt_configcontinuousframemode(CONT_FRAME_PERIOD, 9);
      dwt_writetxfctrl(TX_FRAME_LEN, 0, 0);
      res = transmit(DWT_START_TX_IMMEDIATE);
      if(res != DWT_SUCCESS){
        LOG_ERR("FUCK");
      }
    }
    dwt_writetxdata(HDR_LEN + 2, (uint8_t *) &tx_frame, 0);
    if (c == TX_PER_ROUND){  
      dwt_forcetrxoff();
      k_sleep(K_MSEC(1));
      dwt_rxenable(DWT_START_RX_IMMEDIATE);
      c=0;
    }
    c++;
    //gpio_reset(PORT_DE);
}

uint32_t curr_time;
void start_TX(){
    int res;
    rx_ts_64 = get_rx_timestamp_u64();
    final_tx_time = (rx_ts_64 + (instance_info.tx_dly_us * UUS_TO_DWT_TIME)) >> 8;
    dwt_setdelayedtrxtime(final_tx_time);
    LOG_INF("START TX");
    res = transmit(DWT_START_TX_DELAYED);
    if (res != DWT_SUCCESS){
        LOG_ERR("TX failed");
        return;
    }
    
}

int transmit(uint8_t type){
    int res;
    
    //dwt_configcontinuousframemode(CONT_FRAME_PERIOD, 9);
    
    dwt_writetxdata(HDR_LEN + 2, (uint8_t *) &tx_frame, 0);
    dwt_writetxfctrl(TX_FRAME_LEN, 0, 0);
    res = dwt_starttx(type);
    if (res != DWT_SUCCESS){
      LOG_ERR("TX failed");
      return res;
    }
    
    
    //tx_timestamp = get_tx_timestamp_u64();
    //printk("%ud\n", tx_timestamp);

    return DWT_SUCCESS;
}




static void rx_ok_cb(const dwt_cb_data_t *cb_data){
    dwt_forcetrxoff();
    /* A frame has been received, copy it to our local buffer. */
    dwt_readrxdata((uint8_t *)&rx_frame, HDR_LEN, 0); /* No need to read the FCS/CRC. */
    
    
    switch (rx_frame.type){
    case PACKET_TS:
        //tx_frame.seq = 0;
        start_TX();
    break;
    
    default:
        dwt_forcetrxoff();
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        break;
    }
}