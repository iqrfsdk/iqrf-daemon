#include "IqrfCdcChannel.h"
#include "UdpChannel.h"
#include "helpers.h"
#include "MessageQueue.h"
#include <string>
#include <sstream>

#include <iostream>
#ifdef _DEBUG
#define DEBUG_TRC(msg) std::cout << __FUNCTION__ << ":  " << msg << std::endl;
#define PAR(par)                #par "=\"" << par << "\" "
#else
#define DEBUG_TRC(msg)
#define PAR(par)
#endif

typedef std::basic_string<unsigned char> ustring;

class MessageHandler
{
public:
  MessageHandler(const std::string& portIqrf);
  virtual ~MessageHandler();

  ustring getGwIdent();

  int handleMessageFromUdp(const ustring& udpMessage);
  int handleMessageFromIqrf(const ustring& iqrfMessage);
  void encodeMessageUdp(const ustring& message, ustring& udpMessage);
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

private:
  Channel *m_iqrfChannel;
  Channel *m_udpChannel;

  MessageQueue *m_toUdpMessageQueue;
  MessageQueue *m_toIqrfMessageQueue;
};

#define IQRF_UDP_GW_ADR			        0x20    // 3rd party or user device
#define IQRF_UDP_HEADER_SIZE        9 //header length
#define IQRF_UDP_CRC_SIZE           2 // CRC has 2 bytes

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
  size_t msgSize = udpMessage.size();
  std::cout << "Received from UDP: " << std::endl;
  for (size_t i = 0; i < msgSize; i++) {
    std::cout << "0x" << std::hex << (int)udpMessage[i] << " ";
  }
  std::cout << std::endl;

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
  std::cout << "Sending to UDP: " << std::endl;
  for (int i = 0; i < iqrfMessage.size(); i++) {
    std::cout << "0x" << std::hex << (int)iqrfMessage[i] << " ";
  }
  std::cout << std::endl;

  std::basic_string<unsigned char> udpMessage;
  encodeMessageUdp(iqrfMessage, udpMessage);

  std::cout << "Transmitt to UDP: " << std::endl;
  for (int i = 0; i < udpMessage.size(); i++) {
    std::cout << "0x" << std::hex << (int)udpMessage[i] << " ";
  }
  std::cout << std::endl;

  m_toUdpMessageQueue->pushToQueue(udpMessage);
  return 0;
}

void MessageHandler::encodeMessageUdp(const ustring& message, ustring& udpMessage)
{
  unsigned short dlen = message.size();
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

MessageHandler::MessageHandler(const std::string& portIqrf)
{
  //TODO catch CDCImplException
  m_iqrfChannel = new IqrfCdcChannel(portIqrf);
  
  //TODO ports to param
  m_udpChannel = new UdpChannel(portIqrf);

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

int main(int argc, char** argv)
{
  std::string port_name;

  if (argc < 2) {
    std::cerr << "Usage" << std::endl;
    std::cerr << "  cdc_example <port-name>" << std::endl << std::endl;
    std::cerr << "Example" << std::endl;
    std::cerr << "  cdc_example COM5" << std::endl;
    std::cerr << "  cdc_example /dev/ttyACM0" << std::endl;
    return (-1);
  }
  else {
    port_name = argv[1];
  }

  //TODO report acquired params
  std::cout << "iqrf_gw started" << std::endl;

  //init UDP
  //TODO get port from cmdl
  //IqrfUdpInit(55300, 55000);
  //IqrfUdpOpen();            // Open IQRF UDP socket

  MessageHandler msgHndl(port_name);

  DEBUG_TRC("iqrf_gw main is going to sleep ...");
  while (true)
  {
    //int reclen = IqrfUdpListen();
    //msgHndl.handle(reclen);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }

  //IqrfUdpClose();            // Open IQRF UDP socket
}
