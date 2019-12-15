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
  #define MODEM_RX_BUFFER 255
#endif

#ifndef AT_BUFFER
  #define AT_BUFFER 255
#endif

#define MODEM_TIMEOUT   1000L

#define NL_CHAR     "\r\n"
#define R_OK        "OK" NL_CHAR
#define R_ERROR     "ERROR" NL_CHAR
#define R_CME_ERROR "+CME ERROR: "  //TODO NL_CHAR check if before

#define YIELD() nrf_delay_us(10)

typedef struct {
    bool      connected;
    uint8_t   identifier;
    uint16_t  length;
    uint8_t   content[513];
} socket_t;

socket_t socket = {
  .connected    = false,
  .identifier   = 99,
  .length       =  0,
  .content      = "",
};

typedef const char* ModemConstStr;

char *greetingText = "Hello World";
char _atcommand[AT_BUFFER] = "";        // Static allocate memory to avoid malloc and fragmentation -> sendAT
char _ret[1024 + 1] = "";               // Static allocate memory to avoid malloc and fragmentation -> readStringUntil
bool _isConnected = false;

char* readStringUntil(char terminator);
uint32_t millis(void);
void reboot(void);
void sendAT(char* arg);
void iot_closeSocket(uint8_t _socket);

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
  
  char resp[MODEM_RX_BUFFER] = "";
  uint8_t index = 0;
  unsigned long startMillis = millis();
  do {
    YIELD();
    while (uart_available())
    {
      YIELD();

      resp[strlen(resp)] = uart_read();
      
      // Remove leading \r and \n for pretty NRF_LOF_INFO
      if(resp[0] == '\r' && resp[1] == '\n') 
      {
        memset(resp, '\0', sizeof(resp));
        resp[strlen(resp)] = uart_read();
      }
      // MQTT has a leading \r
      if(resp[0] == '\r' && resp[1] == '\r' && resp[2] == '\n') 
      {
        memset(resp, '\0', sizeof(resp));
        resp[strlen(resp)] = uart_read();
      }

        if(strstr(resp, data) != NULL){
        index = 1;
        goto finish;
      } else if (strstr(resp, r2) != NULL) {
        index = 2;
        goto finish;
      } else if (strstr(resp, r3) != NULL) {
        index = 3;
        strcat(resp, readStringUntil('\n'));
        goto finish;
      } else if (strstr(resp, "+UUSORD: ") != NULL) {
        socket.identifier = atoi(readStringUntil(','));
        socket.length     = atoi(readStringUntil('\r'));
        NRF_LOG_INFO("### URC Data Received: %i on %i", socket.length, socket.identifier);
        NRF_LOG_FLUSH();
      } else if (strstr(resp, "+UUSOCL: ") != NULL) {
        uint8_t _s = uart_read() - '0';
        socket.connected = false;
        NRF_LOG_INFO("### URC Sock Closed: %i", _s);
        NRF_LOG_FLUSH();
      }
    }
  } while ((millis() - startMillis ) < timeout_ms);

  finish: ;

  uint16_t resp_size = strlen(resp);

  if (!index) {
    if (resp_size){
      NRF_LOG_INFO("### Unhandled: %s", resp);
      NRF_LOG_FLUSH();
    }else{
      NRF_LOG_INFO("### TIMEOUT");
      NRF_LOG_FLUSH();
    }
    uart_clear();
  }
  else{
    if(strlen(_buff) > 0)
    {
      // Remove leading and trailing "\n\r" for Logging
      char dbg_buff[1024] = "";

      if (_buff[0] == '\r' && _buff[1] == '\n')
        strncpy(dbg_buff, _buff + 2, strlen(_buff) - 8 - 2);

      NRF_LOG_INFO("<- %s", dbg_buff);
      NRF_LOG_FLUSH();
    }
    else
    {
      // Remove leading and trailing "\n\r" for Logging
      char* _resp = strtok(resp, "\r");
      NRF_LOG_INFO("<- %s", _resp);
      NRF_LOG_FLUSH();
    }
  }

  return index;
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
  //char *_ret = (char*)calloc(1025, sizeof(char));
  memset(_ret, '\0', sizeof(_ret));

  unsigned long startMillis = millis();
  
  while ((millis() - startMillis) < 10000)
  {
      _ret[strlen(_ret)] = uart_read();
      if(_ret[strlen(_ret)-1] == terminator) break;
  }

  if( terminator != ',' )
    uart_clear();                         // Flush uart to remove OK from buffer

  return _ret;
  
}

/**
* @brief Create and connect to the socket
*
* @param host and port.
* */
void iot_connSocket(char* _host, uint16_t _port)
{
  char _command[255] = "";

  //iot_closeSocket( 0 );
  //uart_clear();

  sendAT("+UDCONF=1,1");                                  // Enable HEX mode
  waitResponse(MODEM_TIMEOUT, R_OK);

  sendAT("+USOCR=6");                                     // Create TCP Socket
  waitResponse(MODEM_TIMEOUT, "+USOCR: ");
  socket.identifier = atoi(readStringUntil('\r'));

  nrf_delay_ms(100); // breath

  sprintf(_command, "+USOSEC=%i,1,0", socket.identifier);
  sendAT(_command);                                       // Enable SSL on TCP on socket open previously
  waitResponse(MODEM_TIMEOUT, R_OK);

  nrf_delay_ms(100); // breath

  sprintf(_command, "+USOCO=%i,\"%s\",%d", socket.identifier, _host, _port);
  sendAT(_command);                                       // Connect to the Socket
  if(waitResponse(120000L, R_OK) == 1)
    socket.connected = true;
  else
  {
    iot_closeSocket(socket.identifier);
    uart_clear();
  }

}

/**
* @brief Write to an open TCP socket.
*
* @param data to write and size.
*
* @return true if succesful.
* */
bool iot_writeSSL(const char* data, uint16_t size)
{
  char _command[512 + 20] = "";
  sprintf(_command, "+USOWR=%i,%i,\"", socket.identifier, size);
    
  // Convert data command into HEX
  for (uint16_t i = 0; i < size; i++) {
    char _c[3];
    sprintf(_c, "%02X", data[i]);     // TODO check memory and time resources.
    strcat(_command, _c);
  }

  strcat(_command, "\"");
  sendAT(_command);

  sprintf(_command, "+USOWR: %d,%d" NL_CHAR NL_CHAR R_OK, socket.identifier, size); // Example: +USOWR: 0,512\r\n\r\nOK\r\n
  if(waitResponse(120000L, _command) == 1)
    return true;
   
  else
    return false;
}

/**
* @brief Close the TCP socket.
*
* @param socket to be closed.
* */
void iot_closeSocket(uint8_t _socket)
{
  char closeSocket[8 + 1] = "";
  sprintf(closeSocket, "+USOCL=%i", _socket);

  sendAT(closeSocket);

  if(waitResponse(120000, R_OK) == 1)
    socket.connected = false;
}

/**
* @brief Read data from the socket.
*
* @return true if succesful.
* */
bool iot_readSocket(void)
{
  waitResponse(120000L, "+UUSORD:");  // Example: +UUSORD: 0,1024
  socket.identifier = atoi(readStringUntil(','));  // TODO check if leading space convert to Int alright
  socket.length     = atoi(readStringUntil('\n')); // the same as above \r\n

  char _command[13 + 1]    = "";      // Example AT+USORD=0,1024
  char sock_data[1024 + 1] = "";      // Max socket length is 1024, when HEX == 512

  memset(socket.content, '\0', sizeof(socket.content));

  sprintf(_command, "+USORD=%i,%i", socket.identifier, socket.length);
  sendAT(_command);

  sprintf(_command, "+USORD: %i,%i,\"", socket.identifier, socket.length);
  if( waitResponse(MODEM_TIMEOUT, _command) == 1)
  {
    nrf_delay_ms(500); // breath
    char* _sock_data = readStringUntil('\"');
    strncpy(sock_data, _sock_data, socket.length * 2 );
  }
  else
  {
    iot_closeSocket(socket.identifier);
    return false;
  }

  // Convert HEX char array to String
  for(uint16_t i = 0; i < strlen(sock_data) / 2; i++)
  {
    uint8_t n1 = sock_data[i * 2];
    uint8_t n2 = sock_data[i * 2 + 1];

    if (n1 > '9') {
    n1 = (n1 - 'A') + 10;
    } else {
    n1 = (n1 - '0');
    }

    if (n2 > '9') {
    n2 = (n2 - 'A') + 10;
    } else {
    n2 = (n2 - '0');
    }

    socket.content[i] = (n1 << 4) | n2;
  }

  socket.length = 0;
  return true;
}

/**
* @brief Set MQTT broker configuration
* */
void iotublox_mqtt_config(const char* host, uint16_t port, const char* id,
                          const char* username, const char* password)
{
  char _mqttcommand[18 + 123] = "";                           // Set client ID, max 23 char AT+UMQTT=0,"352753090041680"
  sprintf(_mqttcommand, "+UMQTT=0,\"%s\"", id);
  sendAT(_mqttcommand);
  waitResponse(MODEM_TIMEOUT, "+UMQTT: 0,1\r" NL_CHAR NL_CHAR R_OK);

  sprintf(_mqttcommand, "+UMQTT=2,\"%s\",%i", host, port);  // Set Broker and Port -> AT+UMQTT=2,"www.commercialmqttbroker.com",65535
  sendAT(_mqttcommand);
  waitResponse(MODEM_TIMEOUT, "+UMQTT: 2,1\r" NL_CHAR NL_CHAR R_OK);

  sprintf(_mqttcommand, "+UMQTT=4,\"%s\",\"%s\"", username, password);      // Set username and password -> AT+UMQTT=4,"test","abc123"
  sendAT(_mqttcommand);
  waitResponse(MODEM_TIMEOUT, "+UMQTT: 4,1\r" NL_CHAR NL_CHAR R_OK);
}

/**
* @brief Login and Publish message to the MQTT Broker
*
* @param Payload to be published
*
* @return True if success.
* */
bool iotublox_mqtt_publish(const char* topic, const char* payload, uint8_t QoS)
{
  sendAT("+UMQTTC=1");                                                               // Connect to the Broker
  if(waitResponse(120000L, "+UMQTTC: 1,") != 1 ) return false;
  uint8_t mqtt_res = uart_read() - '0';
  if(mqtt_res != 1)
  {
    NRF_LOG_WARNING("MQTT Connection error #%i", mqtt_res);
    NRF_LOG_FLUSH();
    return false;
  }

  if(waitResponse(120000L, "+UUMQTTC: 1,") !=1 ) return false;                       // Wait for Connection URC
  mqtt_res = atoi(readStringUntil('\r'));
  if(mqtt_res != 0)
  {
    NRF_LOG_WARNING("MQTT URC Connection error #%i", mqtt_res);
    NRF_LOG_FLUSH();
    return false;
  }

  // Convert Payload to HEX as string does not accept " and ,
  char payload_hex[512] = "";
  for (uint16_t i = 0; i < strlen(payload); i++) {
    char _c[3];
    sprintf(_c, "%02X", payload[i]);     // TODO check memory and time resources.
    strcat(payload_hex, _c);
  }
  

  char _mqttcommand[18 + 255] = "";
  //sprintf(_mqttcommand, "+UMQTTC=2,%i,0,\"%s\",\"%s\"", QoS, topic, payload);      // Publish payload to a topic -> AT+UMQTTC=2,0,0,"sensor/heat/SD/bldg5/DelMarConfRm","23degrees Celsius"
  sprintf(_mqttcommand, "+UMQTTC=2,%i,0,1,\"%s\",\"%s\"", QoS, topic, payload_hex);  // Publish payload to a topic in HEX -> AT+UMQTTC=2,0,0,1,"sensor/heat/SD/bldg5/DelMarConfRm","564981A654F"
  sendAT(_mqttcommand);
  if(waitResponse(120000L, "+UMQTTC: 2,1\r" NL_CHAR NL_CHAR R_OK) !=1 ) return false;

  return true;
}

/**
* @brief Disconnect from MQTT broker
* */
void iotublox_mqtt_disconnect(void)
{
  sendAT("+UMQTTC=0");
  waitResponse(120000L, "+UMQTTC: 0," NL_CHAR NL_CHAR R_OK);
  uint8_t mqtt_res = atoi(readStringUntil('\r'));
  if(mqtt_res != 1)
  {
    NRF_LOG_WARNING("MQTT Disonnection error #%d", mqtt_res);
    NRF_LOG_FLUSH();
  }
}

/**
* @return time since MCU powered up in ms.
* */
uint32_t millis(void)
{
  static volatile uint32_t overflows = 0;

  uint64_t ticks = (uint64_t)((uint64_t)overflows << (uint64_t)24) | (uint64_t)(NRF_RTC1->COUNTER);

  return (ticks * 1000) / 16384;

  //return app_timer_cnt_get() / 32.768;
  //return (uint32_t)app_timer_cnt_get() / 16.384;
}

/**
* @brief Function for Sending AT Command.
*
* @param Args Command to be sent.
* */
void sendAT(char* arg)
{
  //char *_atcommand = calloc(strlen(arg) + 4);

    NRF_LOG_INFO("-> %s", arg);
    NRF_LOG_FLUSH();

  sprintf(_atcommand, "AT%s" NL_CHAR, arg);

  uart_write(_atcommand);

  //free(_atcommand);
}

/**
* @brief Function for initializing modem.
*
* @param MNO Profile, RAT and Band Mask.
*
* @return True if success.
* */
bool iotublox_init(uint8_t mnoProf, char* rat, uint64_t bandMask0, uint64_t bandMask1)
{
  sendAT("E0");                                                     // Echo Off.
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;
  
  char _command[AT_BUFFER] = "";
  sprintf(_command, "+CSGT=1,\"%s\"", greetingText);                // Set greeting text to detect when module is ready;
  sendAT(_command);
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+UMNOPROF?");                                             // Check MNO Profile.
  if (waitResponse(MODEM_TIMEOUT, "+UMNOPROF: ") != 1) return false;

  uint8_t _mno = atoi(readStringUntil('\r'));

  if( mnoProf == 0 || _mno == 0 )
  {
    NRF_LOG_WARNING("### WARNING:  default = 0 should not be used!");
    NRF_LOG_FLUSH();
  }
  if( _mno != mnoProf )
  {
    sprintf(_command, "+UMNOPROF=%i", mnoProf);
    sendAT(_command);
    if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;
    
    reboot();
  }
  
  sendAT("+UBANDMASK?");                                            // Check Band Mask, by reducing the bands, connection time reduces significantly
  waitResponse(MODEM_TIMEOUT, "+UBANDMASK: 0,");

  uint64_t _bandMask0 = atoi(readStringUntil(','));
  readStringUntil(',');
  uint64_t _bandMask1 = atoi(readStringUntil('\r'));

  if( _bandMask0 != bandMask0 )
  {
    sprintf(_command, "+UBANDMASK=0,%i", bandMask0);
    sendAT(_command);
    
    waitResponse(MODEM_TIMEOUT, R_OK);
  }
  if( _bandMask1 != bandMask1 )
  {
    sprintf(_command, "+UBANDMASK=1,%i", bandMask1);
    sendAT(_command);
    
    waitResponse(MODEM_TIMEOUT, R_OK);
  }
  
  if( (_bandMask0 != bandMask0) || (_bandMask1 != bandMask1) )  reboot();

  sendAT("+CMEE=2");                                                // Enable Error verbose mode
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  //sendAT("+CEREG=4");                 // Enable URCs on the EPS network registration status
  //if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  //sendAT("+CREG=2");                  // Enable URCs on the GSM network registration status
  //if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+URAT?");                                                 // Check RAT technology (NB-IoT or Cat-M1)
  waitResponse(MODEM_TIMEOUT, "+URAT: ");
  
  // TODO this is a bit ugly -> Accept preferred RAT
  char* _rat   = readStringUntil('\r');
  uint8_t size_rat_read = strlen(_rat) - 1; // remove \r from the end
  uint8_t rat1 = atoi(strtok(_rat, ","));
  uint8_t rat2 = atoi(strtok(NULL, "\r"));

  if( size_rat_read != strlen(rat) )
  {
    sendAT("+CFUN=0");                                              // Turn radio Off
    if (waitResponse(180000L, R_OK) != 1) return false;

    sprintf(_command, "+URAT=%s", rat);                             // Change RAT
    sendAT(_command);
    if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

    sendAT("+CFUN=1");                                              // Turn radio On
    if (waitResponse(180000L, R_OK) != 1) return false;
  }
  else if ( strlen(rat) == 1 )
  {
    if( rat1 != rat[0] - '0' )
    {
      sendAT("+CFUN=0");                                            // Turn radio Off
      if (waitResponse(180000L, R_OK) != 1) return false;

      sprintf(_command, "+URAT=%c", (uint8_t)rat[0]);               // Change RAT
      sendAT(_command);

      if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

      sendAT("+CFUN=1");                                            // Turn radio On
      if (waitResponse(180000L, R_OK) != 1) return false;
    }
  }
    else if ( strlen(rat) == 3 )
  {
    if( rat1 != rat[0] - '0' )
    {
      sendAT("+CFUN=0");                                            // Turn radio Off
      if (waitResponse(180000L, R_OK) != 1) return false;

      sprintf(_command, "+URAT=%s", rat);                           // Change RAT
      sendAT(_command);
      if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

      sendAT("+CFUN=1");                                            // Turn radio On
      if (waitResponse(180000L, R_OK) != 1) return false;
    }
    else if( rat2 != rat[2] - '0' )
    {
      sendAT("+CFUN=0");                                            // Turn radio Off
      if (waitResponse(180000L, R_OK) != 1) return false;

      sprintf(_command, "+URAT=%s", rat);                           // Change RAT
      sendAT(_command);
      if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

      sendAT("+CFUN=1");                                            // Turn radio On
      if (waitResponse(180000L, R_OK) != 1) return false;
    }
  }

  return true;
}

//
// TODO accept Timers in DEC
//
bool iotublox_powerSave(bool upsv, bool psm, const char* tau, const char* active)
{
  if( upsv )  sendAT("+UPSV=4");
  else        sendAT("+UPSV=0");
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  if( !psm )  sendAT("+CPSMS=0");
  else
  {
    char _psm[36];
    sprintf(_psm, "AT+CPSMS=1,,,\"%s\",\"%s\"", tau, active);
    sendAT( _psm );
  }
  if (waitResponse(MODEM_TIMEOUT * 10, R_OK) != 1) return false;

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
  char _command[AT_BUFFER] = "";

  sprintf(_command, "+CGDCONT=1,\"IP\",\"%s\"", apn);    // Define PDP context 1
  sendAT(_command);  
  waitResponse(MODEM_TIMEOUT, R_OK);

  sendAT("+COPS=0");                                    // Automatic operator selection
  if (waitResponse(MODEM_TIMEOUT, R_OK) != 1) return false;

  sendAT("+COPS?");
  unsigned long startMillis = millis();
  while( waitResponse(MODEM_TIMEOUT, "+COPS: 0,0,") != 1 && millis() - startMillis < 30000L)  // TODO try to connect for 30sec if fails -> sleep.
  {
    sendAT("+COPS?");
  }

  nrf_delay_ms(100); // breathe

  uart_clear();

  //sendAT("+CGACT=1,1");                                 // Activate PDP profile/context 1
  //if (waitResponse(150000L, R_OK) != 1) return false;

  sendAT("+CGATT=1");                                   // GPRS Attach 
  if (waitResponse(150000L, R_OK) != 1) return false;

  sendAT("+CGCONTRDP=1");
  waitResponse(150000L, R_OK);

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

  nrf_delay_ms(500);             // breath -> Without it, receiving SIM ERROR;

  sendAT("E0");                  // Echo Off.
  waitResponse(MODEM_TIMEOUT, R_OK);
}

#ifdef __cplusplus
}
#endif

#endif