#include "UdpChannel.h"
#include "IqrfLogging.h"
#include "helpers.h"
#include <exception>

#ifdef _DEBUG
#define THROW_EX(extype, msg) { \
  std::ostringstream ostr; ostr << __FILE__ << " " << __LINE__ << msg; \
  TRC_ERR(ostr.str()); \
  extype ex(ostr.str().c_str()); throw ex; }

#else
#define THROW_EX(extype, msg) { \
  std::ostringstream ostr; ostr << msg; \
  extype ex(ostr.str().c_str()); throw ex; }

#endif

#define IQRF_UDP_GW_ADR			        0x20    // 3rd party or user device
#define IQRF_UDP_CRC_SIZE               2       // CRC has 2 bytes


UdpChannel::UdpChannel(const std::string& portIqrf)
  :m_runListenThread(true)
{
  localPort = 55300;
  remotePort = 55000;
  memset(&m_tx.allbuffer, 0, sizeof(m_tx.allbuffer));
  memset(&m_rx.allbuffer, 0, sizeof(m_rx.allbuffer));

  
  // Initialize Winsock
  WSADATA wsaData = { 0 };
  int iResult = 0;

  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    THROW_EX(std::exception, "WSAStartup failed" << GetLastError());
  }

  memset(&iqrfUdpListener, 0, sizeof(iqrfUdpListener));
  memset(&iqrfUdpTalker, 0, sizeof(iqrfUdpTalker));
  //iqrfUDP.error.openSocket = TRUE;                        // Error

  //iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (iqrfUdpSocket == -1)
    THROW_EX(std::exception, "socket failed" << GetLastError());

  char broadcastEnable = 1;                                // Enable sending broadcast packets
  if (setsockopt(iqrfUdpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) != 0)
  {
    closesocket(iqrfUdpSocket);
    THROW_EX(std::exception, "setsockopt failed" << GetLastError());
  }

  // Remote server, packets are send as a broadcast until the first packet is received
  iqrfUdpTalker.sin_family = AF_INET;
  iqrfUdpTalker.sin_port = htons(remotePort);
  iqrfUdpTalker.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  // Local server, packets are received from any IP
  iqrfUdpListener.sin_family = AF_INET;
  iqrfUdpListener.sin_port = htons(localPort);
  iqrfUdpListener.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(iqrfUdpSocket, (struct sockaddr *)&iqrfUdpListener, sizeof(iqrfUdpListener)) == -1)
  {
    closesocket(iqrfUdpSocket);
    THROW_EX(std::exception, "bind failed" << GetLastError());
  }

  iqrfUdpServerLength = sizeof(iqrfUdpListener);          // Needed for receiving (recvfrom)!!!
  //iqrfUDP.error.openSocket = FALSE;                       // Ok
  //iqrfUDP.taskState = IQRF_UDP_LISTEN;                    // Socket opened

  //iqrfUDP.flag.enabled = TRUE;

  m_listenThread = std::thread(&UdpChannel::listen, this);
}

UdpChannel::~UdpChannel()
{
  //if (iqrfUDP.taskState != IQRF_UDP_CLOSED)               // Socket must be opened to be closed
  {
    //close(iqrfUdpSocket);
    //iqrfUDP.taskState = IQRF_UDP_CLOSED;

    int iResult = closesocket(iqrfUdpSocket);
    if (iResult == SOCKET_ERROR) {
      TRC_DBG("closesocket failed: " << GetLastError());
    }
  }

  if (m_listenThread.joinable())
    m_listenThread.join();

  WSACleanup();
}

void UdpChannel::listen()
{
  /*
  fd_set fds;
  int n;
  struct timeval tv;

  // Set up the file descriptor set.
  FD_ZERO(&fds);
  FD_SET(iqrfUdpSocket, &fds);

  // Set up the struct timeval for the timeout.
  tv.tv_sec = 10;
  tv.tv_usec = 0;

  // Wait until timeout or data received.
  n = select(iqrfUdpSocket, &fds, NULL, NULL, &tv);
  if (n == 0)
  {
  printf("Timeout..\n");
  return 0;
  }
  else if (n == -1)
  {
  printf("Error..\n");
  return 1;
  }

  int length = sizeof(remoteAddr);

  recvfrom(mHandle, buffer, 1024, 0, (SOCKADDR*)&remoteAddr, &length);
  */

  int n = -1;
  uint16_t tmp, dlenRecv, dlenToSend;
  while (m_runListenThread)
  {
    n = recvfrom(iqrfUdpSocket, (char*)m_rx.allbuffer, IQRF_UDP_BUFFER_SIZE, 0, (struct sockaddr *)&iqrfUdpListener, &iqrfUdpServerLength);

    if (n == SOCKET_ERROR) {
      THROW_EX(std::exception, "rcvfrom failed" << WSAGetLastError());
    }

    if (n > 0) {
      std::basic_string<unsigned char> message(m_rx.allbuffer, n);
      if (0 == m_receiveFromFunc(message)) {
        iqrfUdpTalker.sin_addr.s_addr = iqrfUdpListener.sin_addr.s_addr;    // Change the destination to the address of the last received packet
      }
    }
  }
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  TRC_DBG("Send to UDP: " << std::endl << FORM_HEX(message.data(), message.size()));

  int n = sendto(iqrfUdpSocket, (const char*)message.data(), message.size(), 0, (struct sockaddr *)&iqrfUdpTalker, sizeof(iqrfUdpTalker));

  if (n != -1)
  {
    //iqrfUDP.error.sendPacket = FALSE;                   // Ok
    //return 1;                                           // Ok
  }
  else
  {
    //LogError("IqrfUdpSendPacket: sendto() error");
    //return 0;
  }

}

void UdpChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  m_receiveFromFunc = receiveFromFunc;
}
