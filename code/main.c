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

#include "boards.h"
#include "bsp.h"

#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "iotublox.h"
#include "mqtt.h"

//https://devzone.nordicsemi.com/nordic/short-range-guides/b/software-development-kit/posts/application-timer-tutorial
#if NRF_PWR_MGMT_CONFIG_USE_SCHEDULER
#include "app_scheduler.h"
#define APP_SCHED_MAX_EVENT_SIZE    0   /**< Maximum size of scheduler events. */
#define APP_SCHED_QUEUE_SIZE        4   /**< Maximum number of events in the scheduler queue. */
#endif // NRF_PWR_MGMT_CONFIG_USE_SCHEDULER

// TODO for MQTT look here: C:\Users\lbisp\Downloads\nRF5_SDK_16.0.0_98a08e2\examples\iot\mqtt\lwip\publisher

// TODO arduino is not too bad https://github.com/knolleary/pubsubclient/blob/master/src/PubSubClient.cpp

#define MQTT_HOST     "m24.cloudmqtt.com"
#define MQTT_PORT     17603
#define MQTT_PORT_SSL 27603
#define MQTT_ID       "n928y23dj09u20"
#define MQTT_USER     "qvpgdxqg"
#define MQTT_PASS     "YVTESIVDgHlN"

static void sleep_handler(void)
{
    __WFE();
    __SEV();
    __WFE();
}

void URC_handler(void)
{
  NRF_LOG_INFO("URC: %s", _buff);
  NRF_LOG_FLUSH();

  uart_clear();
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    bsp_board_init(BSP_INIT_LEDS);

    nrf_gpio_cfg_input(ARDUINO_6_PIN, NRF_GPIO_PIN_PULLUP);   // SW3, user button
    nrf_gpio_cfg_input(ARDUINO_A2_PIN, NRF_GPIO_PIN_PULLUP);  // V_INT input
    nrf_gpio_cfg_input(BUTTON_2, NRF_GPIO_PIN_PULLUP);        // EVK button
    nrf_gpio_cfg_input(BUTTON_1, NRF_GPIO_PIN_PULLUP);        // EVK button
    nrf_gpio_cfg_output(ARDUINO_A1_PIN);                      // PWR_ON Pin, active High
    
    // Function starting the internal low-frequency clock LFCLK XTAL oscillator.
    // (When SoftDevice is enabled the LFCLK is always running and this is not needed).
    ret_code_t ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    nrf_drv_clock_lfclk_request(NULL);

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

#if NRF_PWR_MGMT_CONFIG_USE_SCHEDULER
    APP_SCHED_INIT(APP_SCHED_MAX_EVENT_SIZE, APP_SCHED_QUEUE_SIZE);
#endif // NRF_PWR_MGMT_CONFIG_USE_SCHEDULER

    //ret = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(ret);

    uart_init();

    static uint8_t welcome[] = "App started\r\n";

    NRF_LOG_INFO("%s", welcome);
    NRF_LOG_FLUSH();

    //uart_write(welcome);

    iotublox_init(100, "8", 524420, 524420);
    iotublox_powerSave(false, false, NULL, NULL);
    iotublox_connect("company.iot.dk1.tdc");
    //iotublox_connect("lpwa.telia.iot");

    while (true)
    {
        if( !nrf_gpio_pin_read(BUTTON_2) )
        {
        
        iot_closeSocket( 0 );
        uart_clear();

          do{
            iot_connSocket("untrol.blynk.cc", 443);
            }while(socket.connected == false);

          //char GET[] = "GET /ENaXVkhOawt3DJm80zrgIiCIkeGxecGF/get/V6 HTTP/1.1\n\rHost: untrol.blynk.cc\n\rConnection: close\n\r\n\r";
          char GET[] = "GET /ENaXVkhOawt3DJm80zrgIiCIkeGxecGF/get/V6 HTTP/1.1\n\r\n\r";
          iot_writeSSL(GET, strlen(GET));
          iot_readSocket();

          //uart_write(socket.content);
          char* response = strstr(socket.content, "\r\n\r\n") + 4;       // Filter HTTP response

          NRF_LOG_INFO("Respons -> %s", response);
          NRF_LOG_FLUSH();

          iot_closeSocket(socket.identifier);

          // MQTT using AT Commands API
          iotublox_mqtt_config(MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_USER, MQTT_PASS);
          iotublox_mqtt_publish("nina/test", "{\"units\": \"MW\", \"value\": 3229, \"ts\": \"05/09/2018 00:05:00\"}", 0);
          iotublox_mqtt_disconnect();

          NRF_LOG_INFO("Finish Button 2");
          NRF_LOG_FLUSH();

          nrf_delay_ms(1000);
        }
/*
        if( !nrf_gpio_pin_read(BUTTON_1) )
        {
          do{
            iot_connSocket("untrol.blynk.cc", 443);
            }while(socket.connected == false);

          iot_connSocket(MQTT_HOST, MQTT_PORT_SSL); // TODO move inside mqtt_connect?
          mqtt_connect(MQTT_ID, MQTT_USER, MQTT_PASS, 0, 0, 0, 0, 1);
          mqtt_publish("nina/test", "02", false);

          nrf_delay_ms(1000);
        }*/

        NRF_LOG_FLUSH();
    }
}


/** @} */
