#include "IQRF_UDP.h"
#include "CdcInterface.h"
#include "CDCImpl.h"
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

void receiveData(unsigned char* data, unsigned int length);

class MessageHandler
{
public:
  MessageHandler(T_IQRF_UDP* iqrfUDP, const std::string& portIqrf);

  virtual ~MessageHandler()
  {
    delete m_cdc;
  }

  void handle(int msglen);
  void sendMsg(int cmd, const std::string& data = std::string());
  void sendMsgReply(const std::string& data = std::string());
  std::string getGwIdent();

  unsigned char* getRawRecData() {
    return iqrfUDP.rxtx.data.pdata.buffer;
  }

  unsigned short getRawRecLen() {
    unsigned short dlenRecv = (iqrfUDP.rxtx.data.header.dlen_H << 8) + iqrfUDP.rxtx.data.header.dlen_L;
    return dlenRecv;
  }

private:
  T_IQRF_UDP* m_iqrfUDP;
  CDCImpl* m_cdc;
  bool m_cdc_valid;

};

MessageHandler::MessageHandler(T_IQRF_UDP* iqrfUDP, const std::string& portIqrf)
  :m_iqrfUDP(iqrfUDP)
  ,m_cdc(nullptr)
  ,m_cdc_valid(false)
{
  try {
    m_cdc = new CDCImpl(portIqrf.c_str());

    bool test = m_cdc->test();

    if (test) {
      std::cout << "Test OK\n";
    }
    else {
      std::cout << "Test FAILED\n";
      //delete testImp;
    }
    m_cdc_valid = true;
  
    // register to receiving asynchronous messages
    AsyncMsgListenerF aml = &receiveData;
    m_cdc->registerAsyncMsgListener(&receiveData);

  }
  catch (CDCImplException& e) {
    std::cout << e.getDescr() << "\n";
  }
}

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
      // sending read temperature request and checking response of the device
      DSResponse dsResponse = m_cdc->sendData(getRawRecData(), getRawRecLen());
      if (dsResponse != OK) {
        // bad response processing...
        std::cout << "Response not OK: " << dsResponse << "\n";
      }
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

void MessageHandler::sendMsg(int cmd, const std::string& data)
{
}

void MessageHandler::sendMsgReply(const std::string& data)
{
  iqrfUDP.rxtx.data.header.cmd |= 0x80;
  memcpy(iqrfUDP.rxtx.data.pdata.buffer, data.c_str(), data.size());
  IqrfUdpSendPacket(data.size());
}

std::string MessageHandler::getGwIdent()
{
  //1. - GW type e.g.: „GW - ETH - 02A“
  //2. - FW version e.g. : „2.50“
  //3. - MAC address e.g. : „00 11 22 33 44 55“
  //4. - TCP / IP Stack version e.g. : „5.42“
  //5. - IP address of GW e.g. : „192.168.2.100“
  //6. - Net BIOS Name e.g. : „iqrf_xxxx “ 15 characters
  //7. - IQRF module OS version e.g. : „3.06D“
  //8. - Public IP address e.g. : „213.214.215.120“
  std::ostringstream os;
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

void receiveData(unsigned char* data, unsigned int length) {
  if (data == NULL || length == 0) {
    std::cout << "No data received\n";
    return;
  }

  printDataInHex(data, length);
  return;
  //std::string recdata((char*)data, length);


  /*
  if (length < HEADER_LENGTH) {
    std::cout << "Unknown data: ";
    printDataInHex(data, length);
    return;
  }

  unsigned char* msgHeader = new unsigned char[HEADER_LENGTH];
  memcpy(msgHeader, data, HEADER_LENGTH);

  if (isConfirmation(msgHeader)) {
    std::cout << "Confirmation received: ";
    printDataInHex(data, length);
    delete[] msgHeader;
    return;
  }

  if (isResponse(msgHeader)) {
    std::cout << "Response received: \n";
    printResponse(data, length);
    delete[] msgHeader;
    return;
  }

  std::cout << "Unknown type of message. Data: ";
  printDataInHex(data, length);
  delete[] msgHeader;
  */
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
  IqrfUdpInit(55300, 55000);
  IqrfUdpOpen();            // Open IQRF UDP socket

  MessageHandler msgHndl(&iqrfUDP, port_name);

  DEBUG_TRC("iqrf_gw is going to listen ...");
  while (iqrfUDP.flag.enabled)
  {
    int reclen = IqrfUdpListen();
    msgHndl.handle(reclen);
  }

  IqrfUdpClose();            // Open IQRF UDP socket
}
