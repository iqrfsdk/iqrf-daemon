#include "UdpChannel.h"
#include "helpers.h"
#include <sstream>
#include <iostream>
#include <exception>

#ifdef _DEBUG
#define DEBUG_TRC(msg) std::cout << __FUNCTION__ << ":  " << msg << std::endl;
#define PAR(par)                #par "=\"" << par << "\" "

#define THROW_EX(extype, msg) { \
  std::ostringstream ostr; ostr << __FILE__ << " " << __LINE__ << msg; \
  DEBUG_TRC(ostr.str()); \
  extype ex(ostr.str().c_str()); throw ex; }

#else
#define DEBUG_TRC(msg)
#define PAR(par)

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
      DEBUG_TRC("closesocket failed: " << GetLastError());
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
    while (true)
    {

      n = recvfrom(iqrfUdpSocket, (char*)m_rx.allbuffer, IQRF_UDP_BUFFER_SIZE, 0, (struct sockaddr *)&iqrfUdpListener, &iqrfUdpServerLength);

      if (n == SOCKET_ERROR) {
        THROW_EX(std::exception, "rcvfrom failed" << WSAGetLastError());
      }

      std::cout << "Received from UDP: " << std::endl;
      for (int i = 0; i < n; i++) {
        std::cout << "0x" << std::hex << (int)m_rx.allbuffer[i] << " ";
      }
      std::cout << std::endl;

      if (n <= 0)
        break;

      // Some data received
      // --- Packet validation ---------------------------------------
      //iqrfUDP.error.receivePacket = TRUE;                     // Error
      // GW_ADR check
      if (m_rx.data.header.gwAddr != IQRF_UDP_GW_ADR)
        break;
      // Min. packet length check
      if (n < (sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE))
        break;
      // DLEN backup
      dlenRecv = (m_rx.data.header.dlen_H << 8) + m_rx.data.header.dlen_L;
      // Max. packet length check
      if ((dlenRecv + sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE) > IQRF_UDP_BUFFER_SIZE)
        break;
      // CRC check
      tmp = (m_rx.data.buffer[dlenRecv] << 8) + m_rx.data.buffer[dlenRecv + 1];

      if (tmp != GetCRC_CCITT(m_rx.allbuffer, dlenRecv + sizeof(T_IQRF_UDP_HEADER)))
        break;

      // --- Packet is valid -----------------------------------------
      //iqrfUDP.error.receivePacket = FALSE;                                // Ok
      iqrfUdpTalker.sin_addr.s_addr = iqrfUdpListener.sin_addr.s_addr;    // Change the destination to the address of the last received packet

      /////////////////////////////////////////
      int msglen = n;
      DEBUG_TRC(PAR(msglen));
      if (msglen <= 0) {
        DEBUG_TRC("receive error: " << PAR(msglen));
        return;
      }

      switch (m_rx.data.header.cmd)
      {
      case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
        sendMsgReply(getGwIdent());
        break;

      case IQRF_UDP_WRITE_IQRF_SPI:       // --- Writes data to the TR module's SPI ---
        m_rx.data.header.subcmd = IQRF_UDP_NAK;
        m_receiveFromFunc(std::basic_string<unsigned char>(m_rx.data.buffer, dlenRecv));
        break;

      default:
        break;
      }
      /////////////////////////////////////////
    }
  }

}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  std::cout << "Sending to UDP: " << std::endl;
  for (int i = 0; i < message.size(); i++) {
    std::cout << "0x" << std::hex << (int)message[i] << " ";
  }
  std::cout << std::endl;

  //iqrfUDP.error.sendPacket = TRUE;

  //if (iqrfUDP.taskState != IQRF_UDP_LISTEN)               // Socket must be listening
  //{
  //  LogError("IqrfUdpSendPacket: socket is not listening");
  //  return 0;
  //}

  unsigned short dlen = message.size();
  memcpy(m_tx.data.buffer, message.data(), dlen);
  // Is there a space for CRC?
  if (message.size() > (sizeof(m_tx.data.buffer) - 2))
  {
    LogError("IqrfUdpSendPacket: too many bytes");
    return;
  }

  m_tx.data.header.gwAddr = IQRF_UDP_GW_ADR;
  m_tx.data.header.dlen_H = (dlen >> 8) & 0xFF;
  m_tx.data.header.dlen_L = dlen & 0xFF;

  uint16_t crc = GetCRC_CCITT(m_tx.allbuffer, dlen + sizeof(T_IQRF_UDP_HEADER));
  m_tx.data.buffer[dlen + sizeof(T_IQRF_UDP_HEADER)] = (crc >> 8) & 0xFF;
  m_tx.data.buffer[dlen + sizeof(T_IQRF_UDP_HEADER) + 1] = crc & 0xFF;

  int tosend = dlen + sizeof(T_IQRF_UDP_HEADER) + IQRF_UDP_CRC_SIZE;

  std::cout << "Transmitt to UDP: " << std::endl;
  for (int i = 0; i < tosend; i++) {
    std::cout << "0x" << std::hex << (int)m_tx.allbuffer[i] << " ";
  }
  std::cout << std::endl;

  int n = sendto(iqrfUdpSocket, (const char*)m_tx.allbuffer, tosend, 0, (struct sockaddr *)&iqrfUdpTalker, sizeof(iqrfUdpTalker));

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

void UdpChannel::sendMsgReply(const std::basic_string<unsigned char>& data)
{
  m_rx.data.header.cmd |= 0x80;
  memcpy(m_rx.data.buffer, data.c_str(), data.size());
  //TODO GW status
  //IqrfUdpSendPacket(data.size());
}

std::basic_string<unsigned char> UdpChannel::getGwIdent()
{
  //1. - GW type e.g.: „GW - ETH - 02A“
  //2. - FW version e.g. : „2.50“
  //3. - MAC address e.g. : „00 11 22 33 44 55“
  //4. - TCP / IP Stack version e.g. : „5.42“
  //5. - IP address of GW e.g. : „192.168.2.100“
  //6. - Net BIOS Name e.g. : „iqrf_xxxx “ 15 characters
  //7. - IQRF module OS version e.g. : „3.06D“
  //8. - Public IP address e.g. : „213.214.215.120“
  std::basic_ostringstream<unsigned char> os;
  os <<
    "udp-daemon-01" << "\x0D\x0A" <<
    "2.50" << "\x0D\x0A" <<
    "00 11 22 33 44 55" << "\x0D\x0A" <<
    "5.42" << "\x0D\x0A" <<
    "192.168.1.11" << "\x0D\x0A" <<
    "iqrf_xxxx" << "\x0D\x0A" <<
    "3.06D" << "\x0D\x0A" <<
    "192.168.1.11" << "\x0D\x0A";

  return os.str();
}
