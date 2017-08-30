/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LaunchUtils.h"
#include "DpaRaw.h"
#include "DpaTransactionTask.h"
#include "UdpMessaging.h"
#include "IDaemon.h"
#include "UdpMessage.h"
#include "IqrfLogging.h"
#include "crc.h"

INIT_COMPONENT(IMessaging, UdpMessaging)

//TODO UdpMessaging works now as any Messaging, but it is not used any Service.
//It behaves as Service, Serializer and Messaging together
//This shall be split in future

void UdpMessaging::sendDpaMessageToUdp(const DpaMessage&  dpaMessage)
{
  ustring message(dpaMessage.DpaPacketData(), dpaMessage.GetLength());
  TRC_DBG(FORM_HEX(message.data(), message.size()));

  std::basic_string<unsigned char> udpMessage(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[cmd] = (unsigned char)IQRF_UDP_IQRF_SPI_DATA;
  encodeMessageUdp(udpMessage, message);
  m_toUdpMessageQueue->pushToQueue(udpMessage);
}

std::unique_ptr<DpaTransaction> UdpMessaging::getDpaTransactionForward(DpaTransaction* forwarded)
{
  if (forwarded != m_operationalTransaction) { 
    std::unique_ptr<DpaTransaction> trn(ant_new UdpMessagingTransaction(this, forwarded));
    sendDpaMessageToUdp(forwarded->getMessage()); //forward request
    return trn;
  }
  else { //returns itself to avoid forwarding in this case
    std::unique_ptr<DpaTransaction> udpTrn(ant_new UdpMessagingTransaction(*m_operationalTransaction));
    return udpTrn;
  }
}

void UdpMessaging::setExclusive(IChannel* chan)
{
  TRC_ENTER("");
  //TODO mutex
  m_exclusiveChannel = chan;
  m_exclusiveChannel->registerReceiveFromHandler([&](const ustring& received)->int {
    sendDpaMessageToUdp(received);
    return 0;
  });
  TRC_LEAVE("");
}

void UdpMessaging::resetExclusive()
{
  TRC_ENTER("");
  m_exclusiveChannel = nullptr;
  TRC_LEAVE("");
}

int UdpMessaging::handleMessageFromUdp(const ustring& udpMessage)
{
  //TRC_DBG("==================================" << std::endl <<
  //  "Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));

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
    //send response
    ustring udpResponse(udpMessage.substr(0, IQRF_UDP_HEADER_SIZE));
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    udpResponse[subcmd] = (unsigned char)IQRF_UDP_ACK;
    encodeMessageUdp(udpResponse);
    //TODO it is required to send back via subcmd write result - implement sync write with appropriate ret code
    m_toUdpMessageQueue->pushToQueue(udpResponse);

    if (m_exclusiveChannel != nullptr) { // exclusiveAccess
      m_exclusiveChannel->sendTo(message);
    }
    else {
      TRC_WAR(std::endl <<
        "****************************************************" << std::endl <<
        "CANNOT SEND DPA MESSAGE IN OPERATIONAL MODE" << std::endl <<
        "****************************************************" << std::endl <<
        "Messages from UDP are accepted only in service mode" << std::endl);
      //m_operationalTransaction->setMessage(message);
      //m_daemon->executeDpaTransaction(*m_operationalTransaction);
    }
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

  uint16_t crc = Crc::get().GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE);
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
  if (crc != Crc::get().GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE))
    THROW_EX(UdpChannelException, "Message has wrong CRC");

  message = udpMessage.substr(IQRF_UDP_HEADER_SIZE, dlen);
}

UdpMessaging::UdpMessaging(const std::string& name)
  : m_udpChannel(nullptr)
  , m_name(name)
  , m_toUdpMessageQueue(nullptr)
{
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

  std::basic_ostringstream<char> ostring;
  ostring << crlf <<
    "iqrf-daemon" << crlf <<
    m_daemon->getDaemonVersion() << crlf <<
    m_udpChannel->getListeningMacAddress() << crlf <<
    "0.00" << crlf << //TODO
    m_udpChannel->getListeningIpAddress() << crlf <<
    "iqrf_xxxx" << crlf << //TODO
    m_daemon->getOsVersion() << "(" << m_daemon->getOsBuild() << ")" << crlf <<
    m_udpChannel->getListeningIpAddress() << crlf; //TODO

  std::string resp = ostring.str();
  ustring res((unsigned char*)resp.data(), resp.size());
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

  m_operationalTransaction = ant_new UdpMessagingTransaction(this);

  TRC_INF("daemon-UDP-protocol started");

  TRC_LEAVE("");
}

void UdpMessaging::stop()
{
  TRC_ENTER("");
  TRC_INF("udp-daemon-protocol stops");
  delete m_operationalTransaction;
  delete m_udpChannel;
  delete m_toUdpMessageQueue;
  TRC_LEAVE("");
}

void UdpMessaging::update(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  m_remotePort = jutils::getPossibleMemberAs<int>("RemotePort", cfg, m_remotePort);
  m_localPort = jutils::getPossibleMemberAs<int>("LocalPort", cfg, m_localPort);
  TRC_LEAVE("");
}

void UdpMessaging::registerMessageHandler(MessageHandlerFunc hndl)
{
  m_messageHandlerFunc = hndl;
}

void UdpMessaging::unregisterMessageHandler()
{
  //TODO not used now from any Service
  m_messageHandlerFunc = IMessaging::MessageHandlerFunc();
}

void UdpMessaging::sendMessage(const ustring& msg)
{
  //TODO not used now from any Service
  m_toUdpMessageQueue->pushToQueue(msg);
}
