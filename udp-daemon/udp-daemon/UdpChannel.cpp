#include "UdpChannel.h"
#include "IqrfLogging.h"
#include "helpers.h"

const unsigned IQRF_UDP_BUFFER_SIZE = 1024;

UdpChannel::UdpChannel(unsigned short remotePort, unsigned short localPort)
  :m_runListenThread(true)
  ,m_remotePort(remotePort)
  ,m_localPort(localPort)
{
  m_rx = new unsigned char[IQRF_UDP_BUFFER_SIZE];
  memset(m_rx, 0, IQRF_UDP_BUFFER_SIZE);
  
  // Initialize Winsock
  WSADATA wsaData = { 0 };
  int iResult = 0;

  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    THROW_EX(UdpChannelException, "WSAStartup failed: " << GetLastError());
  }

  //iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (iqrfUdpSocket == -1)
    THROW_EX(UdpChannelException, "socket failed: " << GetLastError());

  char broadcastEnable = 1;                                // Enable sending broadcast packets
  if (0 != setsockopt(iqrfUdpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)))
  {
    closesocket(iqrfUdpSocket);
    THROW_EX(UdpChannelException, "setsockopt failed: " << GetLastError());
  }

  // Remote server, packets are send as a broadcast until the first packet is received
  memset(&iqrfUdpTalker, 0, sizeof(iqrfUdpTalker));
  iqrfUdpTalker.sin_family = AF_INET;
  iqrfUdpTalker.sin_port = htons(m_remotePort);
  iqrfUdpTalker.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  // Local server, packets are received from any IP
  memset(&iqrfUdpListener, 0, sizeof(iqrfUdpListener));
  iqrfUdpListener.sin_family = AF_INET;
  iqrfUdpListener.sin_port = htons(m_localPort);
  iqrfUdpListener.sin_addr.s_addr = htonl(INADDR_ANY);

  if (-1 == bind(iqrfUdpSocket, (struct sockaddr *)&iqrfUdpListener, sizeof(iqrfUdpListener)))
  {
    closesocket(iqrfUdpSocket);
    THROW_EX(UdpChannelException, "bind failed: " << GetLastError());
  }

  m_listenThread = std::thread(&UdpChannel::listen, this);
}

UdpChannel::~UdpChannel()
{
  int iResult = closesocket(iqrfUdpSocket);
  if (iResult == SOCKET_ERROR) {
    TRC_WAR("closesocket failed: " << GetLastError());
  }

  if (m_listenThread.joinable())
    m_listenThread.join();

  WSACleanup();

  delete[] m_rx;
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

  int recn = -1;
  socklen_t iqrfUdpServerLength = sizeof(iqrfUdpListener);
  
  try {
    while (m_runListenThread)
    {
      recn = recvfrom(iqrfUdpSocket, (char*)m_rx, IQRF_UDP_BUFFER_SIZE, 0, (struct sockaddr *)&iqrfUdpListener, &iqrfUdpServerLength);

      if (recn == SOCKET_ERROR) {
        THROW_EX(UdpChannelException, "rcvfrom failed: " << WSAGetLastError());
      }

      if (recn > 0) {
        std::basic_string<unsigned char> message(m_rx, recn);
        if (0 == m_receiveFromFunc(message)) {
          iqrfUdpTalker.sin_addr.s_addr = iqrfUdpListener.sin_addr.s_addr;    // Change the destination to the address of the last received packet
        }
      }
    }
  }
  catch (UdpChannelException& e) {
    CATCH_EX("listening thread error", UdpChannelException, e);
  }
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  TRC_DBG("Send to UDP: " << std::endl << FORM_HEX(message.data(), message.size()));

  int trmn = sendto(iqrfUdpSocket, (const char*)message.data(), message.size(), 0, (struct sockaddr *)&iqrfUdpTalker, sizeof(iqrfUdpTalker));

  if (trmn < 0) {
    THROW_EX(UdpChannelException, "sendto failed: " << WSAGetLastError());
  }

}

void UdpChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  m_receiveFromFunc = receiveFromFunc;
}
