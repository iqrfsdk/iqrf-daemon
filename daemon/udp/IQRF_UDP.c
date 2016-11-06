/*******************************************************************************
Description

*******************************************************************************/

//******************************************************************************
//          Included Files
//******************************************************************************
//#include <arpa/inet.h>
#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int clientlen_t;
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
typedef int SOCKET;
typedef void * SOCKADDR_STORAGE;
typedef size_t clientlen_t;

#endif

#include "IQRF_UDP.h"
#include "helpers.h"

//******************************************************************************
//          Private Constants
//******************************************************************************
#define IQRF_UDP_GW_ADR			        0x20    // 3rd party or user device
#define IQRF_UDP_CRC_SIZE               2       // CRC has 2 bytes

//******************************************************************************
//          Private Type Definitions
//******************************************************************************

//******************************************************************************
//          Public Variables
//******************************************************************************
T_IQRF_UDP iqrfUDP;

//******************************************************************************
//          Private Variables
//******************************************************************************
//static int iqrfUdpSocket;
static SOCKET iqrfUdpSocket;
static struct sockaddr_in iqrfUdpListener;
static struct sockaddr_in iqrfUdpTalker;
static socklen_t iqrfUdpServerLength;

//******************************************************************************
//          Private Function Prototypes
//******************************************************************************

//******************************************************************************
//          Private Macros
//******************************************************************************

////////////////////////////////////////////////////////////////////////////////
/*******************************************************************************
Function:       void IqrfUdpInit(void)

Description:    Initializes the IQRF UDP module.
This function is called during application initialization.

PreCondition:   None

Input:          None

Output:         None

Side Effects:   None

Typical Usage:  <code>
IqrfUdpInit();
IqrfUdpOpen();

MainLoop()
{
...
if(iqrfUDP.flag.enabled)
IqrfUdpTask();
...
}
</code>

Remarks:        None
*******************************************************************************/
void IqrfUdpInit(int localPort, int remotePort)
{
  memset(&iqrfUDP, 0, sizeof(iqrfUDP));
  iqrfUDP.localPort = localPort;
  iqrfUDP.remotePort = remotePort;
}

/*******************************************************************************
Function:       void IqrfUdpOpen(void)

Description:    Opens UDP socket.

PreCondition:   None

Input:          None

Output:         None

Side Effects:   None

Typical Usage:  <code>
IqrfUdpOpen();
</code>

Remarks:        None
*******************************************************************************/
void IqrfUdpOpen(void)
{
  if (iqrfUDP.taskState != IQRF_UDP_CLOSED)               // Socket must be closed to be opened
    return;

  // Initialize Winsock
  WSADATA wsaData = { 0 };
  int iResult = 0;
    
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    LogError("WSAStartup failed");
    return;
  }

  memset(&iqrfUdpListener, 0, sizeof(iqrfUdpListener));
  memset(&iqrfUdpTalker, 0, sizeof(iqrfUdpTalker));
  iqrfUDP.error.openSocket = TRUE;                        // Error

  //iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (iqrfUdpSocket == -1)
    return;                                             // Socket open error

  int broadcastEnable = 1;                                // Enable sending broadcast packets
  if (setsockopt(iqrfUdpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) != 0)
  {
    //close(iqrfUdpSocket);
    closesocket(iqrfUdpSocket);
    return;                                             // Socket open error
  }
  // Remote server, packets are send as a broadcast until the first packet is received
  iqrfUdpTalker.sin_family = AF_INET;
  iqrfUdpTalker.sin_port = htons(iqrfUDP.remotePort);
  iqrfUdpTalker.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  // Local server, packets are received from any IP
  iqrfUdpListener.sin_family = AF_INET;
  iqrfUdpListener.sin_port = htons(iqrfUDP.localPort);
  iqrfUdpListener.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(iqrfUdpSocket, (struct sockaddr *)&iqrfUdpListener, sizeof(iqrfUdpListener)) == -1)
  {
    close(iqrfUdpSocket);
    return;                                             // Socket open error
  }

  iqrfUdpServerLength = sizeof(iqrfUdpListener);          // Needed for receiving (recvfrom)!!!
  iqrfUDP.error.openSocket = FALSE;                       // Ok
  iqrfUDP.taskState = IQRF_UDP_LISTEN;                    // Socket opened

  iqrfUDP.flag.enabled = TRUE;
}

/*******************************************************************************
Function:       void IqrfUdpClose(void)

Description:    Closes UDP socket.

PreCondition:   None

Input:          None

Output:         None

Side Effects:   None

Typical Usage:  <code>
IqrfUdpClose();
</code>

Remarks:        None
*******************************************************************************/
void IqrfUdpClose(void)
{
  if (iqrfUDP.taskState != IQRF_UDP_CLOSED)               // Socket must be opened to be closed
  {
    //close(iqrfUdpSocket);
    iqrfUDP.taskState = IQRF_UDP_CLOSED;
  
    int iResult = closesocket(iqrfUdpSocket);
    if (iResult == SOCKET_ERROR) {
      LogError("closesocket failed with error");
      WSACleanup();
      return 1;
    }
  }

  WSACleanup();
}

/*******************************************************************************
Function:       uint8_t IqrfUdpSendPacket(uint16_t dlen)

Description:    TODO

PreCondition:   None

Input:          dlen - Number of bytes in DATA part of UDP packet.

Output:         1 - Packet is sent
0 - Packet is not sent:
= The socket is not listening
= Too many bytes for sending
= Sending to the socket was not successful

Side Effects:   None

Typical Usage:  <code>
memset(iqrfUDP.rxtx.buffer, 0, sizeof(iqrfUDP.rxtx.buffer));
iqrfUDP.rxtx.data.header.cmd = IQRF_UDP_IQRF_SPI_DATA;
memcpy(iqrfUDP.rxtx.data.pdata.buffer, (void *)iqrfDPA.request.buffer, dlen);
IqrfUdpSendPacket(dlen);
</code>

Remarks:        None
*******************************************************************************/
uint8_t IqrfUdpSendPacket(uint16_t dlen)
{
  iqrfUDP.error.sendPacket = TRUE;

  if (iqrfUDP.taskState != IQRF_UDP_LISTEN)               // Socket must be listening
  {
    LogError("IqrfUdpSendPacket: socket is not listening");
    return 0;
  }
  // Is there a space for CRC?
  if (dlen > (sizeof(iqrfUDP.rxtx.data.pdata.buffer) - 2))
  {
    LogError("IqrfUdpSendPacket: too many bytes");
    return 0;
  }

  iqrfUDP.rxtx.data.header.gwAddr = IQRF_UDP_GW_ADR;
  iqrfUDP.rxtx.data.header.dlen_H = (dlen >> 8) & 0xFF;
  iqrfUDP.rxtx.data.header.dlen_L = dlen & 0xFF;

  uint16_t crc = GetCRC_CCITT(iqrfUDP.rxtx.buffer, dlen + sizeof(T_IQRF_UDP_HEADER));
  iqrfUDP.rxtx.buffer[dlen + sizeof(T_IQRF_UDP_HEADER)] = (crc >> 8) & 0xFF;
  iqrfUDP.rxtx.buffer[dlen + sizeof(T_IQRF_UDP_HEADER) + 1] = crc & 0xFF;

  //printf("\nsent: \n%s", iqrfUDP.rxtx.data.pdata.buffer);
  printf("\ntrm: \n");
  int tosend = dlen + sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE;
  for (int i = 0; i < tosend; i++)
    printf("%02x ", iqrfUDP.rxtx.buffer[i]);
  printf("\n");

  int n = sendto(iqrfUdpSocket, iqrfUDP.rxtx.buffer, dlen + sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE, 0, (struct sockaddr *)&iqrfUdpTalker, sizeof(iqrfUdpTalker));

  if (n != -1)
  {
    iqrfUDP.error.sendPacket = FALSE;                   // Ok
    return 1;                                           // Ok
  }
  else
  {
    LogError("IqrfUdpSendPacket: sendto() error");
    return 0;
  }
}

uint16_t GetGwIdent(uint8_t* data);

/*******************************************************************************
Function:       void IqrfUdpTask(void)

Description:    TODO

PreCondition:   None

Input:          None

Output:         None

Side Effects:   None

Typical Usage:  <code>
MainLoop()
{
...
if(iqrfUDP.flag.enabled)
IqrfUdpTask();
...
}
</code>

Remarks:        None
*******************************************************************************/
void IqrfUdpTask(void)
{
  int n;
  uint8_t subcmdRecv;
  uint16_t tmp, dlenRecv, dlenToSend;

  switch (iqrfUDP.taskState)
  {
  case IQRF_UDP_CLOSED:

    break;

  case IQRF_UDP_LISTEN:

    n = recvfrom(iqrfUdpSocket, iqrfUDP.rxtx.buffer, IQRF_UDP_BUFFER_SIZE, 0, (struct sockaddr *)&iqrfUdpListener, &iqrfUdpServerLength);
    
    if (n == SOCKET_ERROR) {
      printf("rcvfrom failed:%d\n", WSAGetLastError());
      break;
    }
    
    printf("\nrec: \n");
    for (int i = 0; i < n; i++)
      printf("%02x ", iqrfUDP.rxtx.buffer[i]);
    printf("\n");

    if (n <= 0)
      break;
    // Some data received
    // --- Packet validation ---------------------------------------
    iqrfUDP.error.receivePacket = TRUE;                     // Error
    // GW_ADR check
    if (iqrfUDP.rxtx.data.header.gwAddr != IQRF_UDP_GW_ADR)
      break;
    // Min. packet length check
    if (n < (sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE))
      break;
    // DLEN backup
    dlenRecv = (iqrfUDP.rxtx.data.header.dlen_H << 8) + iqrfUDP.rxtx.data.header.dlen_L;
    // Max. packet length check
    if ((dlenRecv + sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE) > IQRF_UDP_BUFFER_SIZE)
      break;
    // CRC check
    tmp = (iqrfUDP.rxtx.data.pdata.buffer[dlenRecv] << 8) + iqrfUDP.rxtx.data.pdata.buffer[dlenRecv + 1];

    if (tmp != GetCRC_CCITT(iqrfUDP.rxtx.buffer, dlenRecv + sizeof(T_IQRF_UDP_HEADER)))
      break;

    // --- Packet is valid -----------------------------------------
    iqrfUDP.error.receivePacket = FALSE;                                // Ok
    iqrfUdpTalker.sin_addr.s_addr = iqrfUdpListener.sin_addr.s_addr;    // Change the destination to the address of the last received packet
    subcmdRecv = iqrfUDP.rxtx.data.header.subcmd;           // SUBCMD backup
    // Prepare default values, sent PAC_ID must be the same like received
    iqrfUDP.rxtx.data.header.subcmd = 0;
    dlenToSend = 0;

    switch (iqrfUDP.rxtx.data.header.cmd)
    {
    case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
      //TODO
      dlenToSend = GetGwIdent(iqrfUDP.rxtx.data.pdata.buffer);
      //LogError("GetGwIdent");
      break;

    case IQRF_UDP_WRITE_IQRF_SPI:       // --- Writes data to the TR module's SPI ---
      iqrfUDP.rxtx.data.header.subcmd = IQRF_UDP_NAK;

      // TODO
      /*
      if ((appData.operationMode != APP_OPERATION_MODE_SERVICE) || (iqrfSPI.trState != TR_STATE_COMM_MODE))
        break;

      if (IqrfSpiDataWrite(iqrfUDP.rxtx.data.pdata.buffer, dlenRecv, TR_CMD_DATA_WRITE_READ))
        iqrfUDP.rxtx.data.header.subcmd = IQRF_UDP_ACK;
      */
      break;
    }

    iqrfUDP.rxtx.data.header.cmd |= 0x80;
    IqrfUdpSendPacket(dlenToSend);
    break;
  }
}

//------------------------------------------------------
int IqrfUdpListen(void)
{
  int n = -1;
  uint16_t tmp, dlenRecv, dlenToSend;

  switch (iqrfUDP.taskState)
  {
  case IQRF_UDP_CLOSED:

    break;

  case IQRF_UDP_LISTEN:

    n = recvfrom(iqrfUdpSocket, iqrfUDP.rxtx.buffer, IQRF_UDP_BUFFER_SIZE, 0, (struct sockaddr *)&iqrfUdpListener, &iqrfUdpServerLength);

    if (n == SOCKET_ERROR) {
      printf("rcvfrom failed:%d\n", WSAGetLastError());
      break;
    }

    printf("\nrec: \n");
    for (int i = 0; i < n; i++)
      printf("%02x ", iqrfUDP.rxtx.buffer[i]);
    printf("\n");

    if (n <= 0)
      break;
    // Some data received
    // --- Packet validation ---------------------------------------
    iqrfUDP.error.receivePacket = TRUE;                     // Error
    // GW_ADR check
    if (iqrfUDP.rxtx.data.header.gwAddr != IQRF_UDP_GW_ADR)
      break;
    // Min. packet length check
    if (n < (sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE))
      break;
    // DLEN backup
    dlenRecv = (iqrfUDP.rxtx.data.header.dlen_H << 8) + iqrfUDP.rxtx.data.header.dlen_L;
    // Max. packet length check
    if ((dlenRecv + sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE) > IQRF_UDP_BUFFER_SIZE)
      break;
    // CRC check
    tmp = (iqrfUDP.rxtx.data.pdata.buffer[dlenRecv] << 8) + iqrfUDP.rxtx.data.pdata.buffer[dlenRecv + 1];

    if (tmp != GetCRC_CCITT(iqrfUDP.rxtx.buffer, dlenRecv + sizeof(T_IQRF_UDP_HEADER)))
      break;

    // --- Packet is valid -----------------------------------------
    iqrfUDP.error.receivePacket = FALSE;                                // Ok
    iqrfUdpTalker.sin_addr.s_addr = iqrfUdpListener.sin_addr.s_addr;    // Change the destination to the address of the last received packet

    break;

  default:
    break;
  }
  
  if (iqrfUDP.error.receivePacket)
    return -1;
  else
    return n;
}

uint16_t GetGwIdent(uint8_t* data)
{
    //1. - GW type e.g.: „GW - ETH - 02A“
    //2. - FW version e.g. : „2.50“
    //3. - MAC address e.g. : „00 11 22 33 44 55“
    //4. - TCP / IP Stack version e.g. : „5.42“
    //5. - IP address of GW e.g. : „192.168.2.100“
    //6. - Net BIOS Name e.g. : „iqrf_xxxx “ 15 characters
    //7. - IQRF module OS version e.g. : „3.06D“
    //8. - Public IP address e.g. : „213.214.215.120“
  uint16_t retval = sprintf(data,
    "udp-daemon-01\x0D\x0A"
    "2.50\x0D\x0A"
    "00 11 22 33 44 55\x0D\x0A"
    "5.42\x0D\x0A"
    "192.168.1.11\x0D\x0A"
    "iqrf_xxxx\x0D\x0A"
    "3.06D\x0D\x0A"
    "192.168.1.11"
    );
}

//https://msdn.microsoft.com/en-us/library/windows/desktop/ms740668(v=vs.85).aspx#WSAENETRESET

//******************************************************************************
// End of File
