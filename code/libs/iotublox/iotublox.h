/******************************************************************************
 * IoTublox.h
 * u-blox Cellular IoT AT Commands Example
 * Leonardo Bispo
 * Nov, 2019
 * https://github.com/ldab/IoTublox

 * Distributed as-is; no warranty is given.

******************************************************************************/

#ifndef IOTUBLOX_H
#define IOTUBLOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "nrf_delay.h"
#include "nrf_log.h"

#include "uart_handler.h"

#if !defined( MODEM_RX_BUFFER )
  #define MODEM_RX_BUFFER 64
#endif

#ifndef AT_BUFFER
  #define AT_BUFFER 64
#endif

#define MODEM_MUX_COUNT 7
#define MODEM_TIMEOUT   1000L

// May want to use PROGMEM
#define NL_CHAR "\r"
static char R_OK[]        = "OK" NL_CHAR;
static char R_ERROR[]     = "ERROR" NL_CHAR;
static char R_CME_ERROR[] = NL_CHAR "+CME ERROR:";

#define YIELD() nrf_delay_us(0)

typedef const char* ModemConstStr;

char *greetingText = "\"Hello World\"";
char* serverAdress;
bool _isConnected = false;
unsigned char clientBuf[1500] = "";

bool streamSkipUntil(const char c, const unsigned long timeout_ms);
char* readStringUntil(char terminator);
uint32_t millis(void);
void reboot(void);

/**
* @brief Function for waiting for a response from command.
*
* @param timeout_ms Timeout in ms.
*
* @return Error code.
* */
uint8_t waitResponse(uint32_t timeout_ms, char* data)
{
  ModemConstStr r2 = R_ERROR;
  ModemConstStr r3 = R_CME_ERROR;
  ModemConstStr r4 = NULL;
  ModemConstStr r5 = NULL;
  
  char resp[MODEM_RX_BUFFER] = "";
  int index = 0;
  unsigned long startMillis = millis();
  do {
    YIELD();
    while (uart_available())
    {
      YIELD();

      resp[strlen(resp)] = uart_read();     

      if(strcmp(resp, data) == 0){
        index = 1;
        goto finish;
      } else if (strcmp(resp, r2) == 0) {
        index = 2;
        goto finish;
      } else if (strcmp(resp, r3) == 0) {
        index = 3;
        if (r3 == R_CME_ERROR) {
          streamSkipUntil('\n', MODEM_TIMEOUT);  //TODO // Read out the error
        }
        goto finish;
      } else if (strcmp(resp, r4) == 0) {
        index = 4;
        goto finish;
      } else if (strcmp(resp, r5) == 0) {
        index = 5;
        goto finish;
      } else if (strcmp(resp,"+UUSORD: ") == 0) {
        uint8_t socket = atoi(readStringUntil(','));
        uint8_t length = atoi(readStringUntil('\n'));
        // TODO DATA IS AVAILABLE READ IT
        /*
        if (mux >= 0 && mux < MODEM_MUX_COUNT && sockets[mux]) {
          sockets[mux]->got_data = true;
          sockets[mux]->sock_available = length;
        }*/
        NRF_LOG_INFO("### URC Data Received: %i on %i", length, socket);
      } else if (strcmp(resp,"+UUSOCL: ") == 0) {
        uint8_t socket = atoi(readStringUntil('\n'));
        /*
        if (mux >= 0 && mux < MODEM_MUX_COUNT && sockets[mux]) {
          RAISE FLAG
          sockets[mux]->sock_connected = false;
        }*/
        NRF_LOG_INFO("### URC Sock Closed: %i", socket);
      }
    }
  } while ((millis() - startMillis ) < timeout_ms);
  
  finish:

  if (!index) {
    if (strlen(resp)){
      NRF_LOG_INFO("### Unhandled: %s", resp);
    }
    uart_clear();
  }

  return index;
}

/**
* @brief Function for skipping and find Char
*
* @param c  Char to find.
*
* @return True when found.
* */
bool streamSkipUntil(const char c, const unsigned long timeout_ms)
{
  unsigned long startMillis = millis();
  while (millis() - startMillis < timeout_ms) 
  {
    while (uart_available())
    {
      YIELD();
      char _c = uart_read();
      if (_c == c)
      {
        return true;
      }
    }
  }
  return false;
}

/**
* @brief Function for finding Char
*
* @param terminator Char to find.
*
* @return Char Array.
* */
char *readStringUntil(char terminator)
{
  // TODO ADD TIMEOUT
  char *_ret = (char*)calloc(64, sizeof(char));
  
  do{
    _ret[strlen(_ret)] = uart_read();
  }while (_ret[strlen(_ret)-1] >= 0 && _ret[strlen(_ret)-1] != terminator);

  uart_clear();   // Flush uart to remove OK from buffer

  return _ret;
  
}

/**
* @return time since MCU powered up in ms.
* */
uint32_t millis(void)
{
  static volatile uint32_t overflows = 0;

  uint64_t ticks = (uint64_t)((uint64_t)overflows << (uint64_t)24) | (uint64_t)(NRF_RTC1->COUNTER);

  return (ticks * 1000) / 32768;

  //return app_timer_cnt_get() / 32.768;
}

/**
* @brief Function for Sending AT Command.
*
* @param Args Command to be sent.
* */
void sendAT(char* arg)
{
  char *command = malloc(strlen(arg)+1);

    NRF_LOG_INFO("-> %s", arg);
    NRF_LOG_FLUSH();

  sprintf(command, "AT%s%s", arg, NL_CHAR);

  uart_write(command);
}

/**
* @brief Function for initializing modem.
*
* @param MNO Profile, RAT and Band Mask.
*
* @return True if success.
* */
bool iotublox_init(uint8_t mnoProf, uint8_t rat, uint64_t bandMask)
{
  sendAT("E0");                      // Echo Off.
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;
  
  char command[AT_BUFFER] = "";
  sprintf(command, "+CSGT=1,%s", greetingText);
  sendAT(command);
  //if (waitResponse(1000L, R_OK) != 1) return false;

  sendAT("+UMNOPROF?");                           // Check MNO Profile.
  waitResponse(MODEM_TIMEOUT, "+UMNOPROF: ");

  uint8_t _mno = atoi(readStringUntil('\r'));

  if( mnoProf == 0 || _mno == 0 )
  {
    NRF_LOG_INFO("### WARNING:  default = 0 should not be used!");
    NRF_LOG_FLUSH();
  }
  if( _mno != mnoProf )
  {
      sprintf(command, "+UMNOPROF=%i", mnoProf);
      sendAT(command);
      if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;
      
      reboot();
  }
  
  // TODO Implement 2 arguments for band masks
  sendAT("+UBANDMASK?");                              // Check Band Mask, by reducing the bands, connection time reduces significantly
  waitResponse(MODEM_TIMEOUT, "+UBANDMASK: 0,");

  uint64_t _bandMask0 = atoi(readStringUntil(','));
  //readStringUntil(',');
  //uint64_t _bandMask1 = atoi(readStringUntil('\r'));

  if( _bandMask0 != bandMask )
  {
    sprintf(command, "+UBANDMASK=0,%i,1,%i", bandMask, bandMask);
    sendAT(command);
    
    if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;
    
    reboot();
  }

  sendAT("+UDCONF=1,1");              // Enable HEX mode for sockets
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+CMEE=2");                  // Enable Error verbose mode
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+CEREG=4");                 // Enable URCs on the EPS network registration status
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+CREG=2");                  // Enable URCs on the GSM network registration status
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+URAT?");                   // Check RAT technology
  waitResponse(MODEM_TIMEOUT, "+URAT: ");
  
  // TODO accept 2 RATs
  uint8_t _rat = atoi(readStringUntil('\r'));

  if( _rat != rat )
  {
    sendAT("+CFUN=0");                  // Turn radio Off
    if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

    sprintf(command, "+URAT=%i", rat);  // Change RAT
    sendAT(command);

    if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

    sendAT("+CFUN=1");                  // Turn radio On
    if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;
  }

  return true;
}

/**
* NOT IMPLEMENTED
* */
bool powerSave(bool upsv, bool psm, const char* tau, const char* active)
{
  if( upsv )  sendAT("+UPSV=4");
  else        sendAT("+UPSV=0");
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  if( !psm )  sendAT("+CPSMS=0");
  else
  {
    // AT+CPSMS=1,,,"00000010","00000010" // psm, tau, active
    //sendAT("+CPSMS=1,,,\"", tau, ",\"", active, "\"");
  }
  if (waitResponse(1000L, R_OK) != 1) return false;

  return true;
}

/**
* @brief Set APN and Activate PDP context
*
* @param APN.
*
* @return True when success.
* */
bool iotublox_connect(const char* apn)
{
  char command[AT_BUFFER] = "";

  sprintf(command, "+CGDCONT=1,\"IP\",\"%s\"", apn);    // Define PDP context 1
  sendAT(command);  
  waitResponse(MODEM_TIMEOUT, R_OK);
  
  sendAT("+COPS=0");                                    // Automatic operator selection
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+CGACT=1,1");                                 // Activate PDP profile/context 1
  if (waitResponse(150000L, R_OK) != 1) return false;

  sendAT("+CGATT=1");                                   // GPRS Attach 
  if (waitResponse(150000L, R_OK) != 1) return false;

  return true;
}

/**
* @brief Function for rebooting modem.
* */
void reboot(void)
{
  sendAT("+CFUN=15");
  waitResponse(MODEM_TIMEOUT, R_OK);

  // TODO Wait for V_INT or Greeting URC
  if( waitResponse(15000L, greetingText) == 1)
    uart_clear();

  sendAT("E0");                  // Echo Off.
  waitResponse(MODEM_TIMEOUT, R_OK);
}

#ifdef __cplusplus
}
#endif

#endif