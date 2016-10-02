#include "MessageHandler.h"
#include "IqrfCdcChannel.h"
#include "UdpChannel.h"
#include "helpers.h"
#include "MessageQueue.h"
#include "IqrfLogging.h"

const unsigned char IQRF_UDP_GW_ADR = 0x20;    // 3rd party or user device
unsigned char IQRF_UDP_HEADER_SIZE = 9; //header length
unsigned char IQRF_UDP_CRC_SIZE = 2; // CRC has 2 bytes

unsigned IQRF_UDP_BUFFER_SIZE = 1024;

//--- IQRF UDP commands (CMD) ---
const unsigned char IQRF_UDP_GET_GW_INFO = 0x01;	// Returns GW identification
const unsigned char IQRF_UDP_GET_GW_STATUS = 0x02;	// Returns GW status
const unsigned char IQRF_UDP_WRITE_IQRF_SPI = 0x03;	// Writes data to the TR module's SPI
const unsigned char IQRF_UDP_IQRF_SPI_DATA = 0x04;	// Data from TR module's SPI (async)

//--- IQRF UDP subcommands (SUBCMD) ---
const unsigned char IQRF_UDP_ACK = 0x50;	// Positive answer
const unsigned char IQRF_UDP_NAK = 0x60;	// Negative answer
const unsigned char IQRF_UDP_BUS_BUSY = 0x61;	// Communication channel (IQRF SPI or RS485) is busy

//--- IQRF UDP header ---
enum UdpHeader
{
  gwAddr,
  cmd,
  subcmd,
  res0,
  res1,
  pacid_H,
  pacid_L,
  dlen_H,
  dlen_L
};

int MessageHandler::handleMessageFromUdp(const ustring& udpMessage)
{
  TRC_DBG("Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));

  size_t msgSize = udpMessage.size();
  std::basic_string<unsigned char> message;
  decodeMessageUdp(udpMessage, message);

  switch (udpMessage[cmd])
  {
  case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
  {
    std::basic_string<unsigned char> udpMessage;
    encodeMessageUdp(getGwIdent(), udpMessage);
    m_toUdpMessageQueue->pushToQueue(udpMessage);
  }
  break;

  case IQRF_UDP_WRITE_IQRF_SPI:       // --- Writes data to the TR module's SPI ---
    //udpMessage[subcmd] = IQRF_UDP_NAK;
    m_toIqrfMessageQueue->pushToQueue(message);
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
  encodeMessageUdp(iqrfMessage, udpMessage);
  m_toUdpMessageQueue->pushToQueue(udpMessage);
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
  bool valid = false;
  unsigned short dlen = 0;
  while (true) {

    // Min. packet length check
    if (udpMessage.size() < IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE)
      break;

    // GW_ADR check
    if (udpMessage[gwAddr] != IQRF_UDP_GW_ADR)
      break;

    //iqrf data length
    dlen = (udpMessage[dlen_H] << 8) + udpMessage[dlen_L];

    // Max. packet length check
    if ((dlen + IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE) > IQRF_UDP_BUFFER_SIZE)
      break;

    // CRC check
    unsigned short crc = (udpMessage[IQRF_UDP_HEADER_SIZE + dlen] << 8) + udpMessage[IQRF_UDP_HEADER_SIZE + dlen + 1];
    if (crc != GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE))
      break;

    valid = true;
    break;
  }

  message = udpMessage.substr(IQRF_UDP_HEADER_SIZE, dlen);
}

MessageHandler::MessageHandler(const std::string& iqrf_port_name, const std::string& remote_ip_port, const std::string& local_ip_port)
{
  m_iqrfChannel = new IqrfCdcChannel(iqrf_port_name);

  unsigned long int remotePort = strtoul(remote_ip_port.c_str(), nullptr, 0);
  if (0 == remotePort || ULONG_MAX == remotePort)
    remotePort = 55000;
  unsigned long int localPort = strtoul(local_ip_port.c_str(), nullptr, 0);
  if (0 == localPort || ULONG_MAX == localPort)
    localPort = 55000;

  m_udpChannel = new UdpChannel((unsigned short)remotePort, (unsigned short)localPort);

  //Messages from IQRF are sent via MessageQueue to UDP channel
  m_toUdpMessageQueue = new MessageQueue(m_udpChannel);

  //Messages from UDP are sent via MessageQueue to IQRF channel
  m_toIqrfMessageQueue = new MessageQueue(m_iqrfChannel);

  //Received messages from IQRF channel are pushed to UDP MessageQueue
  m_iqrfChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromIqrf(msg); });

  //Received messages from IQRF channel are pushed to IQRF MessageQueue
  m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromUdp(msg); });

}

MessageHandler::~MessageHandler()
{
}

std::basic_string<unsigned char> MessageHandler::getGwIdent()
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

void MessageHandler::watchDog()
{
  TRC_DBG("iqrf_gw main is going to sleep ...");
  while (true)
  {
    //TODO
    //watch worker threads
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }
}

