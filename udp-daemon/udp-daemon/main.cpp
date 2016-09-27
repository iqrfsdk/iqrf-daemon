//#include "IQRF_UDP.h"
#include "IqrfCdcChannel.h"
#include "UdpChannel.h"
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

  void handle(int msglen);
  void sendMsg(int cmd, const ustring& data = ustring());
  void sendMsgReply(const ustring& data = ustring());
  ustring getGwIdent();

  void receiveData(unsigned char* data, unsigned int length);


private:
  Channel *m_iqrfChannel;
  Channel *m_udpChannel;

  MessageQueue *m_toUdpMessageQueue;
  MessageQueue *m_toIqrfMessageQueue;

  //T_IQRF_UDP* m_iqrfUDP;
};

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
  m_iqrfChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) {
    m_toUdpMessageQueue->pushToQueue(msg); });

  //Received messages from IQRF channel are pushed to IQRF MessageQueue
  m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) {
    m_toIqrfMessageQueue->pushToQueue(msg); });

}

MessageHandler::~MessageHandler()
{
}

#if 0
void MessageHandler::handle(int msglen)
{
  DEBUG_TRC(PAR(msglen));
  if (msglen <= 0) {
    DEBUG_TRC("receive error: " << PAR(msglen));
    return;
  }

  switch (m_iqrfUDP->rxtx.data.header.cmd)
  {
  case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
    sendMsgReply(getGwIdent());
    break;

  case IQRF_UDP_WRITE_IQRF_SPI:       // --- Writes data to the TR module's SPI ---
    iqrfUDP.rxtx.data.header.subcmd = IQRF_UDP_NAK;


    try {
      m_toIqrfMessageQueue->pushToQueue(ustring(getRawRecData(), getRawRecLen()));
    }
    catch (CDCSendException& ex) {
      std::cout << ex.getDescr() << "\n";
      // send exception processing...
    }
    catch (CDCReceiveException& ex) {
      std::cout << ex.getDescr() << "\n";
      // receive exception processing...
    }

    // TODO
    /*
    if ((appData.operationMode != APP_OPERATION_MODE_SERVICE) || (iqrfSPI.trState != TR_STATE_COMM_MODE))
    break;

    if (IqrfSpiDataWrite(iqrfUDP.rxtx.data.pdata.buffer, dlenRecv, TR_CMD_DATA_WRITE_READ))
    iqrfUDP.rxtx.data.header.subcmd = IQRF_UDP_ACK;
    */
    break;
  }

}


void MessageHandler::sendMsg(int cmd, const ustring& data)
{
}

void MessageHandler::sendMsgReply(const ustring& data)
{
  iqrfUDP.rxtx.data.header.cmd |= 0x80;
  memcpy(iqrfUDP.rxtx.data.pdata.buffer, data.c_str(), data.size());
  IqrfUdpSendPacket(data.size());
}

ustring MessageHandler::getGwIdent()
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


// prints specified data onto standard output in hex format
void printDataInHex(unsigned char* data, unsigned int length) {
  for (int i = 0; i < length; i++) {
    std::cout << "0x" << std::hex << (int)*data;
    data++;
    if (i != (length - 1)) {
      std::cout << " ";
    }
  }
  std::cout << std::dec << "\n";
}
#endif

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
