#include "DpaTask.h"
#include "IqrfCdcChannel.h"
#include "DpaHandler.h"
#include "DpaTransaction.h"

//#include "UdpChannel.h"
//#include "UdpMessage.h"
#include "helpers.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"
#include <string>
#include <sstream>
#include <thread>

TRC_INIT("");

class IqrfAppDpaTransaction : public DpaTransaction
{
public:
  IqrfAppDpaTransaction(DpaTask& dpaTask);
  virtual ~IqrfAppDpaTransaction();
  virtual const DpaMessage& getMessage() const { return m_dpaTask.getRequest(); }
  virtual void processConfirmationMessage(const DpaMessage& confirmation);
  virtual void processResponseMessage(const DpaMessage& response);
  virtual void processFinished(bool success);

  bool isSuccess() { return m_success; }
private:
  DpaTask& m_dpaTask;
  bool m_success;
};


IqrfAppDpaTransaction::IqrfAppDpaTransaction(DpaTask& dpaTask)
  :m_dpaTask(dpaTask)
{
}

IqrfAppDpaTransaction::~IqrfAppDpaTransaction()
{
}

void IqrfAppDpaTransaction::processConfirmationMessage(const DpaMessage& confirmation)
{
  TRC_WAR("Received confirmation from IQRF: " << std::endl <<
    FORM_HEX(confirmation.DpaPacketData(), confirmation.Length()));
}

void IqrfAppDpaTransaction::processResponseMessage(const DpaMessage& response)
{
  TRC_WAR("Received response from IQRF: " << std::endl <<
    FORM_HEX(response.DpaPacketData(), response.Length()));
  m_dpaTask.parseResponse(response);
}

void IqrfAppDpaTransaction::processFinished(bool success)
{
  m_success = success;
}

int main()
{
  std::cout << "iqrfapp started" << std::endl;

  IChannel* dpaInterface(nullptr);
  DpaHandler* dpaHandler(nullptr);

  try {
    dpaInterface = new IqrfCdcChannel("COM5");

    DpaHandler* dpaHandler = new DpaHandler(dpaInterface);
    //dpaHandler->RegisterAsyncMessageHandler(std::bind(&DpaLibraryDemo::UnexpectedMessage,
    //  this,
    //  std::placeholders::_1));

    dpaHandler->Timeout(100);    // Default timeout is infinite

    unsigned short i = 100;
    //wait for a while, there could be some unread message in CDC
    std::this_thread::sleep_for(std::chrono::seconds(1));

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
        TRC_DBG("executing DpaThermometer ...");
        DpaThermometer therm(0);
        IqrfAppDpaTransaction trans(therm);
        dpaHandler->ExecuteDpaTransaction(trans);
        if (trans.isSuccess())
          std::cout << NAME_PAR(Temperature, therm.getTemperature()) << std::endl;
        else
          std::cout << "Failed to read Temperature at: " << NAME_PAR(addres, therm.getAddress()) << std::endl;
        break;
      }
      case 2:
      {
        TRC_DBG("executing DpaRawMessaging ...");
        //temperatureRequest
        std::basic_string<unsigned char> message = { 0x01, 0x00, 0x0A, 0x00, 0xFF, 0xFF };
        DpaMessage dpaMsg;
        dpaMsg.AddDataToBuffer(message.data(), message.length());
        DpaRawTask raw(dpaMsg);
        IqrfAppDpaTransaction trans(raw);
        dpaHandler->ExecuteDpaTransaction(trans);
        break;
      }
      case 3:
      {
        //pulse LEDR 1
        std::basic_string<unsigned char> message = { 0x01, 0x00, 0x06, 0x03, 0xFF, 0xFF };
        DpaMessage dpaMsg;
        dpaMsg.AddDataToBuffer(message.data(), message.length());
        DpaRawTask raw(dpaMsg);
        IqrfAppDpaTransaction trans(raw);
        dpaHandler->ExecuteDpaTransaction(trans);
        break;
      }
      default:
        break;
      }
    }

  }
  catch (std::exception& ae) {
    std::cout << "There was an error during DPA handler creation: " << ae.what() << std::endl;
  }

  delete dpaHandler;
  delete dpaInterface;

  std::cout << "exited" << std::endl;
  system("pause");
}

#if 0
void encodeMessageUdp(unsigned char command, unsigned char subcommand, const ustring& message, ustring& udpMessage)
{
  static unsigned short pacid = 0;
  pacid++;
  unsigned short dlen = (unsigned short)message.size();
  udpMessage.resize(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[gwAddr] = IQRF_UDP_GW_ADR;
  udpMessage[cmd] = command;
  udpMessage[subcmd] = subcommand;
  udpMessage[pacid_H] = (unsigned char)((pacid >> 8) & 0xFF);
  udpMessage[pacid_L] = (unsigned char)(pacid & 0xFF);
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

  //Received messages from IQRF channel are pushed to IQRF MessageQueueChannel
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
#endif
