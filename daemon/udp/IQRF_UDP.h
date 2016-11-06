/*******************************************************************************
Description

*******************************************************************************/
#ifndef _IQRF_UDP_H
#define _IQRF_UDP_H

//******************************************************************************
//          Included Files
//******************************************************************************
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
//          Public Constants
//******************************************************************************
#define IQRF_UDP_BUFFER_SIZE        512

//--- IQRF UDP commands (CMD) ---
#define IQRF_UDP_GET_GW_INFO		0x01	// Returns GW identification
#define IQRF_UDP_GET_GW_STATUS      0x02	// Returns GW status
#define IQRF_UDP_WRITE_IQRF_SPI		0x03	// Writes data to the TR module's SPI
#define IQRF_UDP_IQRF_SPI_DATA		0x04	// Data from TR module's SPI (async)

//--- IQRF UDP subcommands (SUBCMD) ---
#define IQRF_UDP_ACK			0x50	// Positive answer
#define IQRF_UDP_NAK			0x60	// Negative answer
#define IQRF_UDP_BUS_BUSY		0x61	// Communication channel (IQRF SPI or RS485) is busy

//******************************************************************************
//          Public Type Definitions
//******************************************************************************
//--- IQRF UDP task states ---
typedef enum
{
  IQRF_UDP_CLOSED = 0,
  IQRF_UDP_LISTEN
} T_IQRF_UDP_STATES;

//--- IQRF UDP task flags ---
typedef struct
{
  unsigned char enabled : 1;
} T_IQRF_UDP_FLAGS;

//--- IQRF UDP task errors ---
typedef struct
{
  uint8_t openSocket : 1;
  uint8_t sendPacket : 1;
  uint8_t receivePacket : 1;    // TODO asi zrusit nebo by mohl slouzit k indikaci ze se tam nekdo dobyva
} T_IQRF_UDP_ERRORS;

//--- IQRF UDP header ---
typedef struct
{
  uint8_t gwAddr;
  uint8_t cmd;
  uint8_t subcmd;
  uint8_t res0;
  uint8_t res1;
  uint8_t pacid_H;
  uint8_t pacid_L;
  uint8_t dlen_H;
  uint8_t dlen_L;
} T_IQRF_UDP_HEADER;

//--- IQRF UDP data part ---
typedef struct
{
  uint8_t buffer[(IQRF_UDP_BUFFER_SIZE - sizeof(T_IQRF_UDP_HEADER))];
} T_IQRF_UDP_PDATA;

//--- IQRF UDP Rx/Tx packet ---
typedef union
{
  uint8_t buffer[IQRF_UDP_BUFFER_SIZE];

  struct
  {
    T_IQRF_UDP_HEADER header;
    T_IQRF_UDP_PDATA pdata;
  } data;

} T_IQRF_UDP_RXTXDATA;

//--- Task data structure ---
typedef struct
{
  T_IQRF_UDP_RXTXDATA rxtx;
  uint16_t localPort;
  uint16_t remotePort;
  T_IQRF_UDP_STATES taskState;
  T_IQRF_UDP_FLAGS flag;
  T_IQRF_UDP_ERRORS error;
} T_IQRF_UDP;

//******************************************************************************
//          Public Variables
//******************************************************************************
extern T_IQRF_UDP iqrfUDP;

//******************************************************************************
//          Public Functions Prototypes
//******************************************************************************
void    IqrfUdpInit(int localPort, int remotePort);
void    IqrfUdpOpen(void);
void    IqrfUdpClose(void);
uint8_t IqrfUdpSendPacket(uint16_t dlen);
int     IqrfUdpListen(void);

void    IqrfUdpTask(void);

//******************************************************************************
//          Public Macros
//******************************************************************************

#ifdef __cplusplus
}
#endif

#endif // _IQRF_UDP_H
//******************************************************************************
// End of File
