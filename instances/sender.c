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
uint64_t tx_timestamp;



/* Continuous frame duration, in milliseconds. */
#define CONT_FRAME_DURATION_MS 120000

static packet_t tx_frame = {
    .type = PACKET_DATA,
    .src = 0,
    .seq = 0,
    .dst = 0xffff,
    .len = TX_FRAME_LEN
};

static packet_t rx_frame;
ts_info_t * ts_info = NULL;


LOG_MODULE_REGISTER(SENDER);

void start_TX();
int transmit(uint8_t type);
void start_tx_continues();

/* Declaration of static functions. */
static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_to_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);
static void tx_conf_cb(const dwt_cb_data_t *cb_data);

static int8_t uCurrentTrim_val;
int cnt = 0;
static void start_sending_data(void *a1, void* a2, void*a3){
    int res;
    //k_sleep(K_MSEC(100));
    //start_TX();
    k_sleep(K_MSEC(ts_info->tx_session_duration - 100));
    //LOG_INF("DONE TX %d", cnt);
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


int res;


uint32_t curr_time;
void start_TX(){
    int res;
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
    
    
    dwt_writetxfctrl(TX_FRAME_LEN, 0, 0);
    res = dwt_starttx(type);
    dwt_writetxdata(HDR_LEN + 2, (uint8_t *) &tx_frame, 0);
    if (res != DWT_SUCCESS){
      LOG_ERR("TX failed");
      return res;
    }
    

    return DWT_SUCCESS;
}


static void tx_conf_cb(const dwt_cb_data_t *cb_data){
    if (tx_frame.seq != 0xffff)
        tx_frame.seq++;

    if (tx_frame.seq == ts_info->tx_packet_num){
       dwt_forcetrxoff();
       return;
    }

    tx_timestamp = get_tx_timestamp_u64();
    tx_timestamp +=  (1900 * UUS_TO_DWT_TIME);
    tx_timestamp = tx_timestamp >> 8;
    dwt_setdelayedtrxtime(tx_timestamp);
    res = transmit(DWT_START_TX_DELAYED);
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
    dwt_readrxdata((uint8_t *)&rx_frame, 17, 0); /* No need to read the FCS/CRC. */
    switch (rx_frame.type){
    case PACKET_TS:
        TS_recevied++;
        tx_frame.seq = 0;
        ts_info = (ts_info_t *) rx_frame.payload;
        tx_timestamp = get_rx_timestamp_u64();
        tx_timestamp +=  (1200 * UUS_TO_DWT_TIME);
        tx_timestamp = tx_timestamp >> 8;
        dwt_setdelayedtrxtime(tx_timestamp);
        res = transmit(DWT_START_TX_DELAYED);
        if (res != DWT_SUCCESS){
            LOG_ERR("TX failed");
            return;
        }

        //LOG_INF("TS_INFO: NUM: %d, wait %ld", ts_info->tx_packet_num, ts_info->tx_session_duration);
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