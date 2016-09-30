#include "IQRF_UDP.h"
#include <string>
#include <sstream>
#include <thread>

#include <iostream>
#ifdef _DEBUG
#define DEBUG_TRC(msg) std::cout << __FUNCTION__ << ":  " << msg << std::endl;
#define PAR(par)                #par "=\"" << par << "\" "
#else
#define DEBUG_TRC(msg)
#define PAR(par)
#endif

class MessageHandler
{
public:
  MessageHandler(T_IQRF_UDP* iqrfUDP)
    :m_iqrfUDP(iqrfUDP)
    ,m_cmd_counter(0)
  {}

  virtual ~MessageHandler()
  {}

  void handle(int msglen);
  void sendMsg(unsigned char cmd, unsigned char subcmd, const std::string& data = std::string());
  void sendMsgReply(const std::string& data = std::string());
 
  unsigned char* getRawRecData() {
    return iqrfUDP.rxtx.data.pdata.buffer;
  }

  unsigned short getRawRecLen() {
    unsigned short dlenRecv = (iqrfUDP.rxtx.data.header.dlen_H << 8) + iqrfUDP.rxtx.data.header.dlen_L;
    return dlenRecv;
  }

private:
  std::thread m_listener;
  void listenThread();

  T_IQRF_UDP* m_iqrfUDP;
  short unsigned m_cmd_counter;
};

void MessageHandler::handle(int msglen)
{
  DEBUG_TRC(PAR(msglen));
  if (msglen <= 0) {
    DEBUG_TRC("receive error: " << PAR(msglen));
    return;
  }

  iqrfUDP.rxtx.data.header.cmd &= 0x7F;
  switch (m_iqrfUDP->rxtx.data.header.cmd)
  {
  case IQRF_UDP_GET_GW_INFO:          // --- Returned GW identification ---
    {
      std::string ident((char*)getRawRecData(), getRawRecLen());
      std::cout << "GW Identification:" << std::endl;
      std::cout << ident << std::endl;
    }
    break;

  case IQRF_UDP_WRITE_IQRF_SPI:       // --- Writes data to the TR module's SPI ---
  {
    std::string ident((char*)getRawRecData(), getRawRecLen());
    std::cout << "Rec data:" << std::endl;
    std::cout << ident << std::endl;

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

void MessageHandler::sendMsg(unsigned char cmd, unsigned char subcmd, const std::string& data)
{
  //20 01 00 00 00 00 06 00 00 52 8f
  m_cmd_counter++;
  iqrfUDP.rxtx.data.header.cmd = cmd;
  iqrfUDP.rxtx.data.header.subcmd = subcmd;
  iqrfUDP.rxtx.data.header.pacid_H = (m_cmd_counter >> 8) & 0xFF;
  iqrfUDP.rxtx.data.header.pacid_L = m_cmd_counter & 0xFF;

  std::ostringstream os;
  memcpy(iqrfUDP.rxtx.data.pdata.buffer, data.c_str(), data.size());
  IqrfUdpSendPacket(data.size());
}

void MessageHandler::sendMsgReply(const std::string& data)
{
  iqrfUDP.rxtx.data.header.cmd |= 0x80;
  IqrfUdpSendPacket(data.size());
}

int main()
{
  std::cout << "iqrf_ide_dummy started" << std::endl;

  IqrfUdpInit(55000, 55300);
  IqrfUdpOpen();            // Open IQRF UDP socket

  MessageHandler msgHndl(&iqrfUDP);

  bool run = true;
  while (run)
  {
    int input = 0;
    std::cout << "command >";
    std::cin >> input;

    switch (input) {
    case 0:
      run = false;
      break;
    case 1:
    {
      if (iqrfUDP.flag.enabled) {
        DEBUG_TRC("sending request for GW Identification ...");
        msgHndl.sendMsg(1,0);
        //20 01 00 00 00 00 06 00 00 52 8f
      }
      DEBUG_TRC("is going to listen ...");
      int reclen = IqrfUdpListen();
      msgHndl.handle(reclen);
      break;
    }
    case 2:
    {
      if (iqrfUDP.flag.enabled) {
        DEBUG_TRC("Write data to TR module ...");
        unsigned char temperatureRequest[] = { 0x01, 0x00, 0x0A, 0x00, 0xFF, 0xFF };
        std::string data((char*)temperatureRequest, 6);
        for (int i = 1; i > 0; i--)
          msgHndl.sendMsg(3, 0, data);
        //20 03 00 00 00 00 06 00 00 52 8f
      }
      DEBUG_TRC("is going to listen ...");
      int reclen = IqrfUdpListen();
      msgHndl.handle(reclen);
      break;
    }
    default:
      break;
    }
  }

  IqrfUdpClose();            // Open IQRF UDP socket
  std::cout << "exited" << std::endl;
  system("pause");
}

//unsigned char temperatureRequest[REQUEST_LENGTH] = { 0x01, 0x00, 0x0A, 0x00, 0xFF, 0xFF };
//
//for (int sendCounter = 0; sendCounter < 10; sendCounter++) {
//  try {
//    // sending read temperature request and checking response of the device
//    DSResponse dsResponse = testImp->sendData(temperatureRequest, REQUEST_LENGTH);
