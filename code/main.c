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

#include "nrf_soc.h"
#include "nrf_pwr_mgmt.h"
#include "nordic_common.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "iotublox.h"
#include "mqtt.h"
#include "shtc3.h"
#include "nrf_drv_saadc.h"

#define MQTT_HOST     "m24.cloudmqtt.com"
#define MQTT_PORT     17603
#define MQTT_PORT_SSL 27603
#define MQTT_ID       "n928y23dj09u20"
#define MQTT_USER     "qvpgdxqg"
#define MQTT_PASS     "YVTESIVDgHlN"

//#define MQTT_HOST     "a2jzp6o827zjxz-ats.iot.us-east-2.amazonaws.com"
//#define MQTT_PORT     17603
//#define MQTT_PORT_SSL 8883
//#define HTTP_PORT_SSL 8443
//#define MQTT_ID       "n928y23dj09u20"
//#define MQTT_USER     ""
//#define MQTT_PASS     ""

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600                                     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    6                                       /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS  128                                      // NOTE changed to match measurement
#define ADC_RES_8BIT                    256                                     /**< Maximum digital value for 10-bit ADC conversion. */
#define ADC_RES_10BIT                   1024                                    /**< Maximum digital value for 10-bit ADC conversion. */
#define ADC_RES_14BIT                   16384                                   /**< Maximum digital value for 14-bit ADC conversion. */                                

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_14BIT) * ADC_PRE_SCALING_COMPENSATION * 2)  // NOTE check resolution at sdk_config.h

static nrf_saadc_value_t adc_buf;

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

/**@brief Function for configuring ADC to do battery level conversion.
 */
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
      // no implementation needed
    }
}

/**@brief Function for handling bsp events.
 */
void bsp_evt_handler(bsp_event_t evt)
{
    uint32_t err_code;
    switch (evt)
    {
        case BSP_EVENT_KEY_0:

            break;

        default:
            return; // no implementation needed
    }
    err_code = bsp_indication_set(BSP_INDICATE_SENT_OK);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for configuring ADC to do battery level conversion.
 */
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(SAADC_CH_PSELP_PSELP_AnalogInput2);    // P0.04 = AnalogIn 2 -> https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fpin.html
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for turning modem Off.
 */
bool sara_pwr_off(void)
{
  nrf_gpio_pin_write(ARDUINO_A1_PIN, 1);
  nrf_delay_ms(1500);
  nrf_gpio_pin_write(ARDUINO_A1_PIN, 0);

  while(nrf_gpio_pin_read(ARDUINO_A2_PIN));

  NRF_LOG_INFO("Turning SARA off");

  return true;
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);    // Start RTT with timestamps
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_evt_handler);

    APP_ERROR_CHECK(err_code);

    nrf_gpio_cfg_input(ARDUINO_A2_PIN, NRF_GPIO_PIN_PULLUP);  // V_INT input
    nrf_gpio_cfg_output(ARDUINO_A1_PIN);                      // PWR_ON Pin, active High
    nrf_gpio_cfg_output(RTS_PIN_NUMBER);

    // Function starting the internal low-frequency clock LFCLK XTAL oscillator.
    // (When SoftDevice is enabled the LFCLK is always running and this is not needed).
    ret_code_t ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    nrf_drv_clock_lfclk_request(NULL);

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    bsp_indication_set(BSP_INDICATE_ADVERTISING_SLOW);

    while( nrf_gpio_pin_read(BUTTON_1) )
    {
      __WFE();
    }

    bsp_indication_set(BSP_INDICATE_ADVERTISING);

    adc_configure();

          twi_init();
          float    temperature; // temperature
          float    humidity;    // relative humidity
          SHTC3_GetTempAndHumiPolling(&temperature, &humidity);
          NRF_LOG_INFO("Temp: " NRF_LOG_FLOAT_MARKER "°C and Hum: " NRF_LOG_FLOAT_MARKER "%%", NRF_LOG_FLOAT(temperature), NRF_LOG_FLOAT(humidity)); NRF_LOG_FLUSH();
          SHTC3_Sleep();

          nrf_saadc_value_t adc_result;
          err_code = nrf_drv_saadc_sample_convert(0, &adc_result);
          APP_ERROR_CHECK(err_code);

          uint16_t batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) +
                                             DIODE_FWD_VOLT_DROP_MILLIVOLTS;
          uint8_t percentage_batt_lvl = battery_level_in_percent((uint16_t)(batt_lvl_in_milli_volts * 3000/4200));

    NRF_LOG_INFO("ADC: %d, Milli %d, Perc %d", adc_result, batt_lvl_in_milli_volts, percentage_batt_lvl); NRF_LOG_FLUSH();

    uart_init();

    // Turn SARA ON
    nrf_gpio_pin_write(ARDUINO_A1_PIN, 1);
    nrf_delay_ms(500);
    nrf_gpio_pin_write(ARDUINO_A1_PIN, 0);
    nrf_delay_ms(4500);

    // INIT Modem
    iotublox_init(100, "7", 524420, 524420);
    iotublox_powerSave(false, false, NULL, NULL);
    iotublox_connect("lpwa.telia.iot");

    bsp_indication_set(BSP_INDICATE_ADVERTISING_SLOW);

    while (true)
    {
        if( !nrf_gpio_pin_read(BUTTON_1) )
        {
        
        iot_closeSocket( 0 );
        uart_clear();

        /***************************** POST Secure Example using AT Commands API*****************************************/
        #ifdef HTTP_PORT_SSL
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
        #endif
          /***********************************************************************************************************/

          /*****************************MQTT Non-Secure using AT Commands API*****************************************/
          bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);

          char _msg[64];
          sprintf(_msg, "T:%.01f, H:%.01f, B:%d, RSRP:%.02f Non Sec API", temperature, humidity, percentage_batt_lvl, modemInfo.RSRP);
          iotublox_mqtt_config(MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_USER, MQTT_PASS);
          iotublox_mqtt_publish("nina/test", _msg, 0);
          iotublox_mqtt_disconnect();

          //nrf_delay_ms(1000);
          /***********************************************************************************************************/

          /*****************************MQTT Non-Secure Socket implementation*****************************************/
          bsp_indication_set(BSP_INDICATE_ADVERTISING_DIRECTED);

          do{
            iot_connSocket(MQTT_HOST, MQTT_PORT);
            }while(socket.connected == false);

          memset(_msg, '\0', sizeof(_msg));
          sprintf(_msg, "T:%.01f, H:%.01f Non Sec Socket", temperature, humidity);

          mqtt_connect(MQTT_ID, MQTT_USER, MQTT_PASS, 0, 0, 0, 0, 1);
          mqtt_publish("nina/test", _msg, false);
          /***********************************************************************************************************/

          //nrf_delay_ms(1000);

        /*****************************MQTT Secure Socket implementation*****************************************/
          bsp_indication_set(BSP_INDICATE_BONDING);

          do{
            iot_connSocketSSL(MQTT_HOST, MQTT_PORT_SSL, CloudMQTT_CERTS[0], CloudMQTT_CERTS[1], CloudMQTT_CERTS[2]);
            //iot_connSocketSSL(MQTT_HOST, MQTT_PORT_SSL, AWS_CERTS[0], AWS_CERTS[1], AWS_CERTS[2]);
            }while(socket.connected == false);

          memset(_msg, '\0', sizeof(_msg));
          sprintf(_msg, "T:%.01f, H:%.01f Sec Socket", temperature, humidity);

          mqtt_connect(MQTT_ID, MQTT_USER, MQTT_PASS, 0, 0, 0, 0, true);
          mqtt_publish("nina/test", _msg, false);
       /***********************************************************************************************************/

          bsp_indication_set(BSP_INDICATE_USER_STATE_1);

          sara_pwr_off();

          nrf_libuarte_async_uninit(&modem_uart);

          nrf_gpio_cfg_default(TX_PIN_NUMBER);
          nrf_gpio_cfg_default(RX_PIN_NUMBER);
          bsp_indication_set(BSP_INDICATE_IDLE);

          nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
        }
        
        NRF_LOG_FLUSH();
        __WFE();
    }

}
