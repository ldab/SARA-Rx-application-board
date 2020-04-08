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
#include "shtc3.h"

//#define MQTT_HOST     "m24.cloudmqtt.com"
//#define MQTT_PORT     17603
//#define MQTT_PORT_SSL 27603
//#define MQTT_ID       "n928y23dj09u20"
//#define MQTT_USER     "qvpgdxqg"
//#define MQTT_PASS     "YVTESIVDgHlN"

#define MQTT_HOST     "a2jzp6o827zjxz-ats.iot.us-east-2.amazonaws.com"
#define MQTT_PORT     17603
#define MQTT_PORT_SSL 8883
#define HTTP_PORT_SSL 8443
#define MQTT_ID       "n928y23dj09u20"
#define MQTT_USER     ""
#define MQTT_PASS     ""

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

    uart_init();

    static uint8_t welcome[] = "App started\r\n";

    NRF_LOG_INFO("%s", welcome);
    NRF_LOG_FLUSH();

          twi_init();
          float    temperature; // temperature
          float    humidity;    // relative humidity
          SHTC3_getID();
          SHTC3_GetTempAndHumiPolling(&temperature, &humidity);
          NRF_LOG_INFO("Temp: " NRF_LOG_FLOAT_MARKER "°C and Hum: " NRF_LOG_FLOAT_MARKER "%%", NRF_LOG_FLOAT(temperature), NRF_LOG_FLOAT(humidity)); NRF_LOG_FLUSH();
          nrf_delay_ms(2000);
          NRF_LOG_INFO("Good night"); NRF_LOG_FLUSH();
          SHTC3_Sleep();

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

        /***************************** POST Secure Example using AT Commands API*****************************************/
          do{
            iot_connSocketSSL(MQTT_HOST, HTTP_PORT_SSL, AWS_CERTS[0], AWS_CERTS[1], AWS_CERTS[2]);
            }while(socket.connected == false);

          char post_content[] = "{ \"message\": \"Hello, world\" }";
          char POST[1024] = "";
          sprintf(POST, "POST /topics/nina/test HTTP/1.1\nHost: %s:%d\nUser-Agent: Arduino/1.0\nContent-Type: application/x-www-form-urlencoded\nContent-Length: %d\n\n%s\n\n", MQTT_HOST, HTTP_PORT_SSL, strlen(post_content), post_content);
          iot_write(POST, strlen(POST));
          iot_readSocket();

          //char* response = strstr(socket.content, "\r\n\r\n") + 4;       // Filter HTTP response

          NRF_LOG_INFO("Respons -> %s", socket.content);
          NRF_LOG_FLUSH();

          iot_closeSocket(socket.identifier);
          /***********************************************************************************************************/

          /*****************************MQTT Non-Secure using AT Commands API*****************************************/
          iotublox_mqtt_config(MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_USER, MQTT_PASS);
          iotublox_mqtt_publish("nina/test", "{\"units\": \"MW\", \"value\": 3229, \"ts\": \"05/09/2018 00:05:00\"}", 0);
          iotublox_mqtt_disconnect();

          nrf_delay_ms(1000);
          /***********************************************************************************************************/

          /*****************************MQTT Non-Secure Socket implementation*****************************************/
          do{
            iot_connSocket(MQTT_HOST, MQTT_PORT);
            }while(socket.connected == false);

          mqtt_connect(MQTT_ID, MQTT_USER, MQTT_PASS, 0, 0, 0, 0, 1);
          mqtt_publish("nina/test", "Merry Christmas!", false);
          /***********************************************************************************************************/

          NRF_LOG_INFO("Finish Button 2");
          NRF_LOG_FLUSH();

          nrf_delay_ms(1000);

        }

        /*****************************MQTT Secure Socket implementation*****************************************/
        if( !nrf_gpio_pin_read(BUTTON_1) )
        {
          do{
            //iot_connSocketSSL(MQTT_HOST, MQTT_PORT_SSL, CloudMQTT_CERTS[0], CloudMQTT_CERTS[1], CloudMQTT_CERTS[2]);
            iot_connSocketSSL(MQTT_HOST, MQTT_PORT_SSL, AWS_CERTS[0], AWS_CERTS[1], AWS_CERTS[2]);
            }while(socket.connected == false);

          mqtt_connect(MQTT_ID, MQTT_USER, MQTT_PASS, 0, 0, 0, 0, true);
          mqtt_publish("nina/test", "This is secure", false);
       /***********************************************************************************************************/

          nrf_delay_ms(1000);
        }

        NRF_LOG_FLUSH();
    }
}
