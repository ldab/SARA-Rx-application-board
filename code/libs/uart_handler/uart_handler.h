/******************************************************************************

 * u-blox Cellular IoT AT Commands Example
 * Leonardo Bispo
 * Dec, 2019
 * https://github.com/ldab/ublox_sara_nina

 * Distributed as-is; no warranty is given.

******************************************************************************/

#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "boards.h"
#include "nrf_libuarte_async.h"
#include "nrf_drv_clock.h"

#include <bsp.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_queue.h"
#include "nrf_delay.h"

#define UART_RX_BUFFER    1024
uint8_t _buff[UART_RX_BUFFER] = "";
uint16_t _iTail = 0;

typedef struct {
    uint8_t * p_data;
    uint32_t length;
} buffer_t;

NRF_LIBUARTE_ASYNC_DEFINE(modem_uart, 0, 0, 0, NRF_LIBUARTE_PERIPHERAL_NOT_USED, 255, 3);

NRF_QUEUE_DEF(buffer_t, m_buf_queue, 10, NRF_QUEUE_MODE_NO_OVERFLOW);     // Queue is used for the TX queue

void uart_event_handler(void * context, nrf_libuarte_async_evt_t * p_evt)
{
    nrf_libuarte_async_t * p_libuarte = (nrf_libuarte_async_t *)context;
    ret_code_t ret;

    switch (p_evt->type)
    {
        case NRF_LIBUARTE_ASYNC_EVT_ERROR:
            bsp_board_led_on(0);
            break;
        case NRF_LIBUARTE_ASYNC_EVT_RX_DATA:
            strncat(_buff, p_evt->data.rxtx.p_data, p_evt->data.rxtx.length);
            //strcat(_buff, p_evt->data.rxtx.p_data);
            nrf_libuarte_async_rx_free(p_libuarte, p_evt->data.rxtx.p_data, p_evt->data.rxtx.length);
            break;
        case NRF_LIBUARTE_ASYNC_EVT_TX_DONE:
            break;
        default:
            break;
    }
}
ret_code_t uart_init(void)
{

  nrf_libuarte_async_config_t nrf_libuarte_async_config = {
    .tx_pin     = TX_PIN_NUMBER,
    .rx_pin     = RX_PIN_NUMBER,
    .baudrate   = NRF_UARTE_BAUDRATE_115200,
    .parity     = NRF_UARTE_PARITY_EXCLUDED,
    .hwfc       = NRF_UARTE_HWFC_DISABLED,
    .timeout_us = 100,
    .int_prio   = APP_IRQ_PRIORITY_LOW
  };

  ret_code_t err_code = nrf_libuarte_async_init(&modem_uart, &nrf_libuarte_async_config, uart_event_handler, (void *)&modem_uart);
  APP_ERROR_CHECK(err_code);

  nrf_libuarte_async_enable(&modem_uart);

  nrf_delay_ms(100);
}

bool uart_clear(void)
{
  memset(_buff, '\0', sizeof(_buff));
  _iTail = 0;
  return true;
}

uint16_t uart_available(void)
{
  uint16_t _d = (uint16_t)(strlen(_buff) - _iTail);

  if(_d < 0)
    return UART_RX_BUFFER + _d;
  else
    return _d;
}

uint8_t uart_read(void)
{
  uint8_t value   = _buff[_iTail];
  uint16_t length = strlen(_buff);
  
  if(!uart_available())
    return -1;

  _iTail = (uint16_t)(_iTail + 1) % UART_RX_BUFFER;

  if(_iTail == length)
  {
    memset(_buff, '\0', sizeof(_buff));
    _iTail = 0;
    return value;
  }

  return value;
}

void uart_write(char *data)
{
  ret_code_t ret;
  size_t size = strlen(data);
  
  while(ret != NRF_SUCCESS) ret = nrf_libuarte_async_tx(&modem_uart, data, size);

  if (ret == NRF_ERROR_BUSY)
  {
      buffer_t buf = {
          .p_data = data,
          .length = size,
      };

      ret = nrf_queue_push(&m_buf_queue, &buf);
      APP_ERROR_CHECK(ret);
  }
  else
  {
      APP_ERROR_CHECK(ret);
  }
  while (!nrf_queue_is_empty(&m_buf_queue))
  {    
      bsp_board_led_invert(1);
      buffer_t _b;
      ret = nrf_queue_pop(&m_buf_queue, &_b);
      APP_ERROR_CHECK(ret);
      UNUSED_RETURN_VALUE(nrf_libuarte_async_tx(&modem_uart, _b.p_data, _b.length));
  }
}

void uart_write_bin(char *data, size_t size)
{
  ret_code_t ret = -1;
  
  while(ret != NRF_SUCCESS) ret = nrf_libuarte_async_tx(&modem_uart, data, size);

  if (ret == NRF_ERROR_BUSY)
  {
      buffer_t buf = {
          .p_data = data,
          .length = size,
      };

      ret = nrf_queue_push(&m_buf_queue, &buf);
      APP_ERROR_CHECK(ret);
  }
  else
  {
      APP_ERROR_CHECK(ret);
  }
  while (!nrf_queue_is_empty(&m_buf_queue))
  {    
      bsp_board_led_invert(1);
      buffer_t _b;
      ret = nrf_queue_pop(&m_buf_queue, &_b);
      APP_ERROR_CHECK(ret);
      UNUSED_RETURN_VALUE(nrf_libuarte_async_tx(&modem_uart, _b.p_data, _b.length));
  }
}

#endif