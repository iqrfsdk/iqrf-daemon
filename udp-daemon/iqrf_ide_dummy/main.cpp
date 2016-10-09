#include "UdpChannel.h"
#include "UdpMessage.h"
#include "helpers.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"
#include <string>
#include <sstream>
#include <thread>

TRC_INIT("");

void encodeMessageUdp(unsigned char command, unsigned char subcommand, const ustring& message, ustring& udpMessage)
{
  unsigned short dlen = (unsigned short)message.size();
  udpMessage.resize(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[gwAddr] = IQRF_UDP_GW_ADR;
  udpMessage[cmd] = command;
  udpMessage[subcmd] = subcommand;
  udpMessage[dlen_H] = (unsigned char)((dlen >> 8) & 0xFF);
  udpMessage[dlen_L] = (unsigned char)(dlen & 0xFF);

  udpMessage.insert(IQRF_UDP_HEADER_SIZE, message);

  uint16_t crc = GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE] = (unsigned char)((crc >> 8) & 0xFF);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE + 1] = (unsigned char)(crc & 0xFF);
}

int handleMessageFromUdp(const ustring& udpMessage)
{
  TRC_WAR("Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));
  return 0;
}

///////////////////////////////
int main()
{
  std::cout << "iqrf_ide_dummy started" << std::endl;

  unsigned short m_remotePort(55300);
  unsigned short m_localPort(55000);

  UdpChannel *m_udpChannel = ant_new UdpChannel(m_remotePort, m_localPort, IQRF_UDP_BUFFER_SIZE);

  //Received messages from IQRF channel are pushed to IQRF MessageQueue
  m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromUdp(msg); });

  bool run = true;
  while (run)
  {
    int input = 0;
    //std::cout << "command >";
    std::cin >> input;

    switch (input) {
    case 0:
      run = false;
      break;
    case 1:
    {
      TRC_DBG("sending request for GW Identification ...");
      ustring udpMessage;
      ustring message;
      encodeMessageUdp(1, 0, message, udpMessage);
      m_udpChannel->sendTo(udpMessage);
      break;
    }
    case 2:
    {
      TRC_DBG("Write data to TR module ...");
      ustring udpMessage;
      //temperatureRequest
      ustring message = { 0x01, 0x00, 0x0A, 0x00, 0xFF, 0xFF };
      encodeMessageUdp(3, 0, message, udpMessage);

      for (int i = 1; i > 0; i--)
        m_udpChannel->sendTo(udpMessage);
      break;
    }
    case 3:
    {
      TRC_DBG("Write data to TR module ...");
      ustring udpMessage;
      //reset coordinator
      ustring message = { 0x00, 0x00, 0x02, 0x01, 0xFF, 0xFF };
      encodeMessageUdp(3, 0, message, udpMessage);

      for (int i = 1; i > 0; i--)
        m_udpChannel->sendTo(udpMessage);
      break;
    }
    default:
      break;
    }
  }

  delete m_udpChannel;
  std::cout << "exited" << std::endl;
  system("pause");
}
