/******************************************************************************

 * u-blox Cellular IoT AT Commands Example
 * Leonardo Bispo
 * Dec, 2019
 * https://github.com/ldab/ublox_sara_nina

 * Distributed as-is; no warranty is given.

******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <bsp.h>

#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"
#include "app_timer.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "boards.h"
#include "iotublox.h"

static void sleep_handler(void)
{
    __WFE();
    __SEV();
    __WFE();
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    bsp_board_init(BSP_INIT_LEDS);

    nrf_gpio_cfg_input(ARDUINO_6_PIN, NRF_GPIO_PIN_PULLUP);   // SW3, user button
    nrf_gpio_cfg_input(ARDUINO_A2_PIN, NRF_GPIO_PIN_PULLUP);  // V_INT input
    nrf_gpio_cfg_input(BUTTON_2, NRF_GPIO_PIN_PULLUP);        // EVK button
    nrf_gpio_cfg_output(ARDUINO_A1_PIN);                      // PWR_ON Pin, active High
    
    ret_code_t ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
  
    nrf_drv_clock_lfclk_request(NULL);

    ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    uart_init();

    static uint8_t welcome[] = "App started\r\n";

    NRF_LOG_INFO("%s", welcome);
    NRF_LOG_FLUSH();

    uart_write(welcome);

    iotublox_init(100, 8, 524420);
    iotublox_connect("lpwa.telia.iot");

    while (true)
    {
        if( !nrf_gpio_pin_read(BUTTON_2) )
        {
          NRF_LOG_INFO("uart_available: %i", uart_available());
          char data[64] = "";
          uint8_t _i = 0;

          while(uart_available())
          {
            data[_i] = uart_read();
            _i++;
          }
          uart_write(data);

          nrf_delay_ms(1000);
        }

        NRF_LOG_FLUSH();
    }
}


/** @} */
