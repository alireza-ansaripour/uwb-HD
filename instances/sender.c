#include "instance.h"
#include "logging/log.h"
#include "deca_regs.h"
#include <deca_device_api.h>
#include "shared_defines.h"
#include "shared_functions.h"
#include "math.h"
#include <sys/printk.h>
#define TX_FRAME_LEN 1000
#define TX_ANT_DLY 16385


#define CONT_FRAME_PERIOD 400000
// #define TX_PER_ROUND      1150
// #define CONT_FRAME_PERIOD 500000
#define TX_PER_ROUND      1500

//#define MODE_CONT_TX      0
#define MODE_WAIT_TS      1


#define MY_STACK_SIZE 500
#define MY_PRIORITY 5


K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);
struct k_thread my_thread_data;


/* Continuous frame duration, in milliseconds. */
#define CONT_FRAME_DURATION_MS 120000

static packet_t tx_frame = {
    .type = PACKET_DATA,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = TX_FRAME_LEN
};





//static void thread_fn(void *a1, void* a2, void*a3){
//    while (1){
//        printk("ALI\n");
//        k_sleep(K_SECONDS(1));
//    }
    
//}

//static void thread_fn2(void *a1, void* a2, void*a3){
//    while (1){
//        printk("ALI2\n");
//        k_sleep(K_SECONDS(1));
//    }
    
//}



static packet_t rx_frame;

LOG_MODULE_REGISTER(SENDER);

void start_TX();
int transmit(uint8_t type);
void start_tx_continues();

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
uint8_t tx_stat = 10;
static int8_t uCurrentTrim_val;
int cnt = 0;
static void start_sending_data(void *a1, void* a2, void*a3){
    int res;
    start_TX();
    k_sleep(K_MSEC(2500));
    LOG_INF("DONE TX %d", cnt);
    cnt++;
    dwt_forcetrxoff();
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

float abs(float num){
    if (num > 0)
        return num;
    return -1 * num;
}


void instance_sender(){
  uint32_t status_reg;
  uint16_t frame_len;
  prev_ts = 0;
  LOG_INF("Starting Sender");
  tx_frame.src = instance_info.addr;
  tx_frame.dst = instance_info.dst_addr;
  instance_init();

  uCurrentTrim_val = dwt_getxtaltrim();

  gpio_set(PORT_DE);
  dwt_settxantennadelay(TX_ANT_DLY);
  dwt_setcallbacks(&tx_conf_cb, &rx_ok_cb,NULL,&rx_err_cb, NULL, NULL);
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
  #ifdef  MODE_WAIT_TS
   dwt_rxenable(DWT_START_RX_IMMEDIATE);
  #else
   start_tx_continues();
  #endif    
}
int c = 1, GXX=0;

uint64_t tx_timestamp;
int res;


uint32_t curr_time;
void start_TX(){
    int res;
    rx_ts_64 = get_rx_timestamp_u64();
    final_tx_time = (rx_ts_64 + (instance_info.tx_dly_us * UUS_TO_DWT_TIME)) >> 8;
    dwt_setdelayedtrxtime(final_tx_time);
    LOG_INF("START TX");
    res = transmit(DWT_START_TX_IMMEDIATE);
    
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
    

    return DWT_SUCCESS;
}


static void tx_conf_cb(const dwt_cb_data_t *cb_data){
    if (tx_frame.seq != 0xffff)
        tx_frame.seq++;

    if (tx_frame.seq == TX_PER_ROUND){
       dwt_forcetrxoff();
       tx_stat = 0;
       return;
    }

    // tx_timestamp = get_tx_timestamp_u64();
    // tx_timestamp +=  (1900 * UUS_TO_DWT_TIME);
    // tx_timestamp = tx_timestamp >> 8;
    // dwt_setdelayedtrxtime(tx_timestamp);
    // dwt_writetxfctrl(TX_FRAME_LEN, 0, 0);
    res = transmit(DWT_START_TX_IMMEDIATE);
    if (res != DWT_SUCCESS){
        LOG_ERR("TX failed");
        return;
    }
    
}


# define TARGET_XTAL_OFFSET_VALUE_PPM_MIN    (2.0f)
# define TARGET_XTAL_OFFSET_VALUE_PPM_MAX    (4.0f)
#define FS_XTALT_MAX_VAL    (XTAL_TRIM_BIT_MASK)

/* The typical trimming range (with 4.7pF external caps is 
 * ~77ppm (-65ppm to +12ppm) over all steps, see DW3000 Datasheet */
#define AVG_TRIM_PER_PPM    ((FS_XTALT_MAX_VAL+1)/77.0f)

uint16_t TS_recevied = 0;
k_tid_t my_tid;

static void rx_ok_cb(const dwt_cb_data_t *cb_data){
    float xtalOffset_ppm;
    dwt_forcetrxoff();
    /* A frame has been received, copy it to our local buffer. */
    dwt_readrxdata((uint8_t *)&rx_frame, HDR_LEN, 0); /* No need to read the FCS/CRC. */
    switch (rx_frame.type){
    case PACKET_TS:
        TS_recevied++;
        if (TS_recevied < 100){
            tx_frame.seq = 0;
        }else{
            tx_frame.seq = 0xffff;
        }
        tx_stat = 1;
        

        my_tid = k_thread_create(&my_thread_data, my_stack_area,
                                 K_THREAD_STACK_SIZEOF(my_stack_area),
                                 start_sending_data,
                                 NULL, NULL, NULL,
                                 MY_PRIORITY, 0, K_NO_WAIT);
        //start_TX();
        
    break;
    
    default:
        dwt_forcetrxoff();
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        break;
    }
}

static void rx_err_cb(const dwt_cb_data_t *cb_data){
    dwt_forcetrxoff();
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}


void start_tx_continues(){
    for (size_t i = 0; i < 100; i++){
        tx_frame.seq = 0;
        dwt_forcetrxoff();
        tx_stat = 1;
        start_TX();
        while(tx_stat == 1){}
        LOG_INF("TX DONE");
    }
    while (1){
        LOG_INF("LAST TX");
        tx_frame.seq = 0xffff;
        dwt_forcetrxoff();
        tx_stat = 1;
        start_TX();
        while(tx_stat == 1){}
        LOG_INF("LAST TX DONE");

    }
    
  
}




        // k_thread_start(animate_ping_tid);
        // xtalOffset_ppm = ((float)dwt_readclockoffset()) * CLOCK_OFFSET_PPM_TO_RATIO * 1e6;

        // if (abs(xtalOffset_ppm) > 2 || abs(xtalOffset_ppm) < 4) {

        //             uCurrentTrim_val -= ((TARGET_XTAL_OFFSET_VALUE_PPM_MAX + 
        //                                   TARGET_XTAL_OFFSET_VALUE_PPM_MIN)/2 +
        //                                   xtalOffset_ppm) * AVG_TRIM_PER_PPM;
        //             uCurrentTrim_val &= FS_XTALT_MAX_VAL;

        //             /* Configure new Crystal Offset value */
        //             dwt_setxtaltrim(uCurrentTrim_val);
        //         }