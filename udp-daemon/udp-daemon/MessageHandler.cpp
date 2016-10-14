#include "MessageHandler.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "UdpChannel.h"
#include "UdpMessage.h"
#include "helpers.h"
#include "MessageQueue.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <climits>

enum IqrfWriteResults {
  wr_OK = 0x50,
  wr_Error_Len = 0x60, //(number of data = 0 or more than TR buffer COM length)
  wr_Error_SPI = 0x61, //(SPI bus busy)
  wr_Error_IQRF = 0x62 //(IQRF - CRCM Error)
};

int MessageHandler::handleMessageFromUdp(const ustring& udpMessage)
{
  TRC_DBG("Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));

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
    std::basic_string<unsigned char> udpResponse(udpMessage);
    encodeMessageUdp(getGwIdent(), udpResponse);
    udpResponse[cmd] = (unsigned char)IQRF_UDP_GET_GW_INFO | 0x80;
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  case IQRF_UDP_WRITE_IQRF:       // --- Writes data to the TR module ---
  {
    m_toIqrfMessageQueue->pushToQueue(message);

    //send response
    std::basic_string<unsigned char> udpResponse(udpMessage.substr(0, IQRF_UDP_HEADER_SIZE));
    std::basic_string<unsigned char> message;
    encodeMessageUdp(message, udpResponse);
    udpResponse[cmd] = (unsigned char)IQRF_UDP_WRITE_IQRF | 0x80;
    //TODO it is required to send back via subcmd write result - implement sync write with appropriate ret code
    udpResponse[subcmd] = (unsigned char)IQRF_UDP_ACK;
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  default:
    break;
  }
  return -1;
}

int MessageHandler::handleMessageFromIqrf(const ustring& iqrfMessage)
{
  TRC_DBG("Received from to IQRF: " << std::endl << FORM_HEX(iqrfMessage.data(), iqrfMessage.size()));
  
  std::basic_string<unsigned char> udpMessage;
  std::basic_string<unsigned char> message(iqrfMessage);
  encodeMessageUdp(message, udpMessage);
  udpMessage[cmd] = (unsigned char)IQRF_UDP_IQRF_SPI_DATA;
  m_toUdpMessageQueue->pushToQueue(udpMessage);
  
  //std::basic_string<unsigned char> udpMessage;
  //encodeMessageUdp(iqrfMessage, udpMessage);
  //m_toUdpMessageQueue->pushToQueue(udpMessage);
  return 0;
}

void MessageHandler::encodeMessageUdp(const ustring& message, ustring& udpMessage)
{
  unsigned short dlen = (unsigned short)message.size();
  udpMessage.resize(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[gwAddr] = IQRF_UDP_GW_ADR;
  udpMessage[dlen_H] = (unsigned char)((dlen >> 8) & 0xFF);
  udpMessage[dlen_L] = (unsigned char)(dlen & 0xFF);

  udpMessage.insert(IQRF_UDP_HEADER_SIZE, message);

  uint16_t crc = GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE] = (unsigned char)((crc >> 8) & 0xFF);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE + 1] = (unsigned char)(crc & 0xFF);
}

void MessageHandler::decodeMessageUdp(const ustring& udpMessage, ustring& message)
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

MessageHandler::MessageHandler(const std::string& iqrf_port_name, const std::string& remote_ip_port, const std::string& local_ip_port)
  :m_iqrfChannel(nullptr)
  , m_udpChannel(nullptr)
  , m_toUdpMessageQueue(nullptr)
  , m_toIqrfMessageQueue(nullptr)
  , m_iqrfPortName(iqrf_port_name)
{
  m_remotePort = strtoul(remote_ip_port.c_str(), nullptr, 0);
  if (0 == m_remotePort || ULONG_MAX == m_remotePort)
    m_remotePort = 55000;
  m_localPort = strtoul(local_ip_port.c_str(), nullptr, 0);
  if (0 == m_localPort || ULONG_MAX == m_localPort)
    m_localPort = 55300;
}

MessageHandler::~MessageHandler()
{
}

std::basic_string<unsigned char> MessageHandler::getGwIdent()
{
  //1. - GW type e.g.: �GW - ETH - 02A�
  //2. - FW version e.g. : �2.50�
  //3. - MAC address e.g. : �00 11 22 33 44 55�
  //4. - TCP / IP Stack version e.g. : �5.42�
  //5. - IP address of GW e.g. : �192.168.2.100�
  //6. - Net BIOS Name e.g. : �iqrf_xxxx � 15 characters
  //7. - IQRF module OS version e.g. : �3.06D�
  //8. - Public IP address e.g. : �213.214.215.120�
  //std::basic_ostringstream<unsigned char> os;
  std::basic_ostringstream<char> ostring;
  ostring <<
    "\x0D\x0A" << "udp-daemon-01" << "\x0D\x0A" <<
    "2.50" << "\x0D\x0A" <<
    "00 11 22 33 44 55" << "\x0D\x0A" <<
    "5.42" << "\x0D\x0A" <<
    "192.168.1.11" << "\x0D\x0A" <<
    "iqrf_xxxx" << "\x0D\x0A" <<
    "3.06D" << "\x0D\x0A" <<
    "192.168.1.11" << "\x0D\x0A";

  ustring res((unsigned char*)ostring.str().data(), ostring.str().size());
  //TRC_DBG("retval:" << PAR(res.size()) << std::endl << FORM_HEX(res.data(),res.size()));
  return res;
}

void MessageHandler::watchDog()
{
  TRC_ENTER("");
  m_running = true;
  try {
    start();
    while (m_running)
    {
      //TODO
      //watch worker threads
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  stop();
  TRC_LEAVE("");
}

void MessageHandler::start()
{
  TRC_ENTER("");
  size_t found = m_iqrfPortName.find("spi");
  if (found != std::string::npos)
    m_iqrfChannel = ant_new IqrfSpiChannel(m_iqrfPortName);
  else
    m_iqrfChannel = ant_new IqrfCdcChannel(m_iqrfPortName);

  m_udpChannel = ant_new UdpChannel((unsigned short)m_remotePort, (unsigned short)m_localPort, IQRF_UDP_BUFFER_SIZE);

  //Messages from IQRF are sent via MessageQueue to UDP channel
  m_toUdpMessageQueue = ant_new MessageQueue(m_udpChannel);

  //Messages from UDP are sent via MessageQueue to IQRF channel
  m_toIqrfMessageQueue = ant_new MessageQueue(m_iqrfChannel);

  //Received messages from IQRF channel are pushed to UDP MessageQueue
  m_iqrfChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromIqrf(msg); });

  //Received messages from UDP channel are pushed to IQRF MessageQueue
  m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromUdp(msg); });

  std::cout << "udp-daemon started" << std::endl;
  TRC_LEAVE("");
}

void MessageHandler::stop()
{
  TRC_ENTER("");
  std::cout << "udp-daemon stops" << std::endl;
  delete m_iqrfChannel;
  delete m_udpChannel;
  delete m_toUdpMessageQueue;
  delete m_toIqrfMessageQueue;
  TRC_LEAVE("");
}

void MessageHandler::exit()
{
  TRC_WAR("exiting ...");
  std::cout << "udp-daemon exits" << std::endl;
  m_running = false;
}
