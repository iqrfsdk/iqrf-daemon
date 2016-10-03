#include "UdpChannel.h"
#include "IqrfLogging.h"
#include "helpers.h"

#include "PlatformDep.h"

UdpChannel::UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned bufsize)
  :m_runListenThread(true)
  ,m_remotePort(remotePort)
  ,m_localPort(localPort)
  ,m_bufsize(bufsize)
{
  m_rx = ant_new unsigned char[m_bufsize];
  memset(m_rx, 0, m_bufsize);
  
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
  TRC_ENTER("thread starts");

  int recn = -1;
  socklen_t iqrfUdpServerLength = sizeof(iqrfUdpListener);
  
  try {
    while (m_runListenThread)
    {
      recn = recvfrom(iqrfUdpSocket, (char*)m_rx, m_bufsize, 0, (struct sockaddr *)&iqrfUdpListener, &iqrfUdpServerLength);

      if (recn == SOCKET_ERROR) {
        THROW_EX(UdpChannelException, "recvfrom failed: " << WSAGetLastError());
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
    m_runListenThread = false;
  }
  TRC_ENTER("thread stopped");
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
