#include "instance.h"
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <port.h>
#include <shared_defines.h>

//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(instance_common);
instance_info_t instance_info; 

/* Default communication configuration. We use default non-STS DW mode. */
dwt_config_t default_config = {
    .chan            = 9,               /* Channel number. */
    .txPreambLength  = DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    .rxPAC           = DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    .txCode          = 9,               /* TX preamble code. Used in TX only. */
    .rxCode          = 9,               /* RX preamble code. Used in RX only. */
    .sfdType         = DWT_SFD_DW_8,    /* 0 to use standard 8 symbol SFD */
    .dataRate        = DWT_BR_6M8,      /* Data rate. */
    .phrMode         = DWT_PHRMODE_EXT, /* PHY header mode. */
    .phrRate         = DWT_PHRRATE_STD, /* PHY header rate. */
    .sfdTO           = (8000 + 8 - 8),   /* SFD timeout */
    .stsMode         = DWT_STS_MODE_OFF,
    .stsLength       = DWT_STS_LEN_64,  /* STS length, see allowed values in Enum dwt_sts_lengths_e */
    .pdoaMode        = DWT_PDOA_M0      /* PDOA mode off */
};
extern dwt_txconfig_t txconfig_options;


void gpio_set(uint16_t port){
	dwt_or16bitoffsetreg(GPIO_OUT_ID, 0, (port));
}

void gpio_reset(uint16_t port){
	dwt_and16bitoffsetreg(GPIO_OUT_ID, 0, (uint16_t)(~(port)));
}

#define SET_OUPUT_GPIOs 0xFFFF & ~(GPIO_DIR_GDP4_BIT_MASK | GPIO_DIR_GDP2_BIT_MASK | GPIO_DIR_GDP3_BIT_MASK)
#define ENABLE_ALL_GPIOS_MASK 0x200000

void init_LEDs(){
	dwt_enablegpioclocks();
	dwt_write32bitoffsetreg(GPIO_MODE_ID, 0, ENABLE_ALL_GPIOS_MASK);
	dwt_write16bitoffsetreg(GPIO_OUT_ID, 0, 0x0);
	dwt_write16bitoffsetreg(GPIO_DIR_ID, 0, SET_OUPUT_GPIOs);

}



int instance_radio_config(){
    if (dwt_configure(&default_config)){
        LOG_ERR("CONFIG FAILED");
        return DWT_ERROR;
    }
    txconfig_options.power = 0xffffffff;
    dwt_configuretxrf(&txconfig_options);
    return DWT_SUCCESS;

}

void instance_init(){
    int res;
    /* Display application name. */
    LOG_INF("Starting instance");

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC(); /* Target specific drive of RSTn line into DW IC low for a period. */

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC,
     *( or could wait for SPIRDY event) */
    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };
    init_LEDs();

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }


    /* Enabling LEDs here for debug so that for each TX the D1 LED will flash
     * on DW3000 red eval-shield boards. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK) ;
    res = instance_radio_config();
    if(res == DWT_ERROR){
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }
    
}