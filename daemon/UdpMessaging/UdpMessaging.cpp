#include "DpaTask.h"
#include "DpaMessage.h"
#include "UdpMessaging.h"
#include "MessagingController.h"
#include "UdpMessage.h"
#include "helpers.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <climits>
#include <ctime>
#include <ratio>
#include <chrono>

void UdpMessaging::sendDpaMessageToUdp(const DpaMessage&  dpaMessage)
{
  ustring message(dpaMessage.DpaPacketData(), dpaMessage.Length());
  TRC_DBG(FORM_HEX(message.data(), message.size()));

  std::basic_string<unsigned char> udpMessage(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[cmd] = (unsigned char)IQRF_UDP_IQRF_SPI_DATA;
  encodeMessageUdp(udpMessage, message);
  m_toUdpMessageQueue->pushToQueue(udpMessage);
}

void UdpMessaging::setDaemon(IDaemon* daemon)
{
  //m_daemon = daemon;
  //m_daemon->registerMessaging(*this);
}

void UdpMessaging::setExclusiveAccess()
{
  if (!m_exclusiveAccess) {
    m_exclusiveAccess = true;
    m_watchDog.start(30000, [&]() {resetExclusiveAccess(); });
    m_messagingController->exclusiveAccess(true);
    m_messagingController->getIqrfInterface()->registerReceiveFromHandler([&](const ustring& received)->int {
      sendDpaMessageToUdp(received);
      return 0;
    });
  }
}

void UdpMessaging::resetExclusiveAccess()
{
  if (m_exclusiveAccess) {
    m_exclusiveAccess = false;
    //m_watchDog.stop();
    m_messagingController->exclusiveAccess(false);
  }
}

int UdpMessaging::handleMessageFromUdp(const ustring& udpMessage)
{
  //TRC_DBG("==================================" << std::endl <<
  //  "Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));

  if (!m_exclusiveAccess)
    setExclusiveAccess();
  else
    m_watchDog.pet();

  size_t msgSize = udpMessage.size();
  std::basic_string<unsigned char> message;

  try {
    decodeMessageUdp(udpMessage, message);
  }
  catch (UdpChannelException& e) {
    TRC_DBG("Wrong message: " << e.what());
    return -1;
  }

  switch (udpMessage[cmd])
  {
  case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
  {
    ustring udpResponse(udpMessage);
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    ustring msg;
    getGwIdent(msg);
    encodeMessageUdp(udpResponse, msg);
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  case IQRF_UDP_GET_GW_STATUS:          // --- Returns GW status ---
  {
    if (!m_exclusiveAccess)
      setExclusiveAccess();
    else
      m_watchDog.pet();

    ustring udpResponse(udpMessage);
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    ustring msg;
    getGwStatus(msg);
    encodeMessageUdp(udpResponse, msg);
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  case IQRF_UDP_WRITE_IQRF:       // --- Writes data to the TR module ---
  {
    m_messagingController->getIqrfInterface()->sendTo(message);
    
    //send response
    ustring udpResponse(udpMessage.substr(0, IQRF_UDP_HEADER_SIZE));
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    udpResponse[subcmd] = (unsigned char)IQRF_UDP_ACK;
    encodeMessageUdp(udpResponse);
    //TODO it is required to send back via subcmd write result - implement sync write with appropriate ret code
    m_toUdpMessageQueue->pushToQueue(udpResponse);

    //m_transaction->setMessage(message);
    //m_messagingController->executeDpaTransaction(*m_transaction);
  }
  return 0;

  default:
    //not implemented command
    std::basic_string<unsigned char> udpResponse(udpMessage);
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    udpResponse[subcmd] = (unsigned char)IQRF_UDP_NAK;
    encodeMessageUdp(udpResponse);
    m_toUdpMessageQueue->pushToQueue(udpResponse);
    break;
  }
  return -1;
}

void UdpMessaging::encodeMessageUdp(ustring& udpMessage, const ustring& message)
{
  unsigned short dlen = (unsigned short)message.size();

  udpMessage.resize(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[gwAddr] = IQRF_UDP_GW_ADR;
  udpMessage[dlen_H] = (unsigned char)((dlen >> 8) & 0xFF);
  udpMessage[dlen_L] = (unsigned char)(dlen & 0xFF);

  if (0 < dlen)
    udpMessage.insert(IQRF_UDP_HEADER_SIZE, message);

  uint16_t crc = GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE] = (unsigned char)((crc >> 8) & 0xFF);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE + 1] = (unsigned char)(crc & 0xFF);
}

void UdpMessaging::decodeMessageUdp(const ustring& udpMessage, ustring& message)
{
  unsigned short dlen = 0;

  // Min. packet length check
  if (udpMessage.size() < IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE)
    THROW_EX(UdpChannelException, "Message is too short: " << FORM_HEX(udpMessage.data(), udpMessage.size()));

  // GW_ADR check
  if (udpMessage[gwAddr] != IQRF_UDP_GW_ADR)
    THROW_EX(UdpChannelException, "Message is has wrong GW_ADDR: " << PAR_HEX(udpMessage[gwAddr]));

  //iqrf data length
  dlen = (udpMessage[dlen_H] << 8) + udpMessage[dlen_L];

  // Max. packet length check
  if ((dlen + IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE) > IQRF_UDP_BUFFER_SIZE)
    THROW_EX(UdpChannelException, "Message is too long: " << PAR(dlen));

  // CRC check
  unsigned short crc = (udpMessage[IQRF_UDP_HEADER_SIZE + dlen] << 8) + udpMessage[IQRF_UDP_HEADER_SIZE + dlen + 1];
  if (crc != GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE))
    THROW_EX(UdpChannelException, "Message has wrong CRC");

  message = udpMessage.substr(IQRF_UDP_HEADER_SIZE, dlen);
}

UdpMessaging::UdpMessaging(MessagingController* messagingController)
  :m_messagingController(messagingController)
  , m_udpChannel(nullptr)
  , m_toUdpMessageQueue(nullptr)
{
  m_exclusiveAccess = false;
  m_remotePort = 55000;
  m_localPort = 55300;

  m_messagingController->registerMessaging(*this);

  //m_remotePort = strtoul(remote_ip_port.c_str(), nullptr, 0);
  //if (0 == m_remotePort || ULONG_MAX == m_remotePort)
  //  m_remotePort = 55000;
  //m_localPort = strtoul(local_ip_port.c_str(), nullptr, 0);
  //if (0 == m_localPort || ULONG_MAX == m_localPort)
  //  m_localPort = 55300;
}

UdpMessaging::~UdpMessaging()
{
}

void UdpMessaging::getGwIdent(ustring& message)
{
  //1. - GW type e.g.: GW - ETH - 02A
  //2. - FW version e.g. : 2.50
  //3. - MAC address e.g. : 00 11 22 33 44 55
  //4. - TCP / IP Stack version e.g. : 5.42
  //5. - IP address of GW e.g. : 192.168.2.100
  //6. - Net BIOS Name e.g. : iqrf_xxxx (15 characters)
  //7. - IQRF module OS version e.g. : 3.06D
  //8. - Public IP address e.g. : 213.214.215.120

  const char* crlf = "\x0D\x0A";
  const std::string& ipAddr = m_udpChannel->getListeningIpAdress();

  //TODO set correct IP adresses, MAC, OS ver, etc
  std::basic_ostringstream<char> ostring;
  ostring << crlf <<
    "udp-dmn-01" << crlf <<
    "1.00" << crlf <<
    "00 00 00 00 00 00" << crlf <<
    "5.42" << crlf <<
    ipAddr << crlf <<
    "iqrf_xxxx" << crlf <<
    "3.06D" << crlf <<
    ipAddr << crlf;

  ustring res((unsigned char*)ostring.str().data(), ostring.str().size());
  message = res;
}

void UdpMessaging::getGwStatus(ustring& message)
{
  // current date/time based on current system
  time_t now = time(0);
  tm *ltm = localtime(&now);

  message.resize(UdpGwStatus::unused12 + 1, '\0');
  //TODO get channel status to Channel iface
  message[trStatus] = 0x80;   //SPI_IQRF_SPI_READY_COMM = 0x80, see spi_iqrf.h
  message[supplyExt] = 0x01;  //DB3 0x01 supplied from external source
  message[timeSec] = (unsigned char)ltm->tm_sec;    //DB4 GW time – seconds(see Time and date coding)
  message[timeMin] = (unsigned char)ltm->tm_min;    //DB5 GW time – minutes
  message[timeHour] = (unsigned char)ltm->tm_hour;    //DB6 GW time – hours
  message[timeWday] = (unsigned char)ltm->tm_wday;    //DB7 GW date – day of the week
  message[timeMday] = (unsigned char)ltm->tm_mday;    //DB8 GW date – day
  message[timeMon] = (unsigned char)ltm->tm_mon;    //DB9 GW date – month
  message[timeYear] = (unsigned char)(ltm->tm_year % 100);   //DB10 GW date – year
}

void UdpMessaging::start()
{
  TRC_ENTER("");

  m_udpChannel = ant_new UdpChannel((unsigned short)m_remotePort, (unsigned short)m_localPort, IQRF_UDP_BUFFER_SIZE);

  m_toUdpMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_udpChannel->sendTo(msg);
  });

  m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromUdp(msg); });

  m_transaction = ant_new UdpMessagingTransaction(this);

  TRC_INF("daemon-UDP-protocol started");

  TRC_LEAVE("");
}

void UdpMessaging::stop()
{
  TRC_ENTER("");
  TRC_INF("udp-daemon-protocol stops");
  delete m_transaction;
  delete m_udpChannel;
  delete m_toUdpMessageQueue;
  TRC_LEAVE("");
}

////////////////////////////////////
UdpMessagingTransaction::UdpMessagingTransaction(UdpMessaging* udpMessaging)
  :m_udpMessaging(udpMessaging)
{
}

UdpMessagingTransaction::~UdpMessagingTransaction()
{
}

const DpaMessage& UdpMessagingTransaction::getMessage() const
{
  return m_message;
}

int UdpMessagingTransaction::getTimeout() const
{
  return -1;
}

void UdpMessagingTransaction::processConfirmationMessage(const DpaMessage& confirmation)
{
  m_udpMessaging->sendDpaMessageToUdp(confirmation);
}

void UdpMessagingTransaction::processResponseMessage(const DpaMessage& response)
{
  m_udpMessaging->sendDpaMessageToUdp(response);
}

void UdpMessagingTransaction::processFinish(DpaRequest::DpaRequestStatus status)
{
  m_success = isProcessed(status);
}

void UdpMessagingTransaction::setMessage(ustring message)
{
  m_message.DataToBuffer(message.data(), message.length());
}
