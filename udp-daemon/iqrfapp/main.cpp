#include "MqChannel.h"
#include "DpaTask.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"
#include <string>

TRC_INIT("");

int handleMessageFromMq(const ustring& udpMessage)
{
  TRC_WAR("Received from MQ: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));
  return 0;
}

int main()
{
  std::cout << "iqrfapp started" << std::endl;

  MqChannel* mqChannel = ant_new MqChannel("iqrf-daemon-110", "iqrf-daemon-100", 1024);

  //Received messages from IQRF channel are pushed to IQRF MessageQueueChannel
  mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  try {
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
        //DpaThermometer therm(0);
        //IqrfAppDpaTransaction trans(therm);
        //dpaHandler->ExecuteDpaTransaction(trans);
        //if (trans.isSuccess())
        //  std::cout << NAME_PAR(Temperature, therm.getTemperature()) << std::endl;
        //else
        //  std::cout << "Failed to read Temperature at: " << NAME_PAR(addres, therm.getAddress()) << std::endl;
        ustring message = { 0x00, 0x00, 0x02, 0x01, 0xFF, 0xFF };
        try {
          mqChannel->sendTo(message);
        }
        catch (std::exception& e) {
          TRC_DBG("sendTo failed: " << e.what());
        }
        break;
      }
      case 2:
      {
        TRC_DBG("executing DpaRawMessaging ...");
        ////temperatureRequest
        //std::basic_string<unsigned char> message = { 0x01, 0x00, 0x0A, 0x00, 0xFF, 0xFF };
        //DpaMessage dpaMsg;
        //dpaMsg.AddDataToBuffer(message.data(), message.length());
        //DpaRawTask raw(dpaMsg);
        //IqrfAppDpaTransaction trans(raw);
        //dpaHandler->ExecuteDpaTransaction(trans);
        break;
      }
      case 3:
      {
        ////pulse LEDR 1
        //std::basic_string<unsigned char> message = { 0x01, 0x00, 0x06, 0x03, 0xFF, 0xFF };
        //DpaMessage dpaMsg;
        //dpaMsg.AddDataToBuffer(message.data(), message.length());
        //DpaRawTask raw(dpaMsg);
        //IqrfAppDpaTransaction trans(raw);
        //dpaHandler->ExecuteDpaTransaction(trans);
        ustring message = { 0x01, 0x00, 0x06, 0x03, 0xFF, 0xFF };
        try {
          mqChannel->sendTo(message);
        }
        catch (std::exception& e) {
          TRC_DBG("sendTo failed: " << e.what());
        }
        break;
      }
      case 4:
      {
        TRC_DBG("executing startPipe() ...");
        break;
      }
      case 5:
      {
        TRC_DBG("executing writePipe() ...");
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

  std::cout << "exited" << std::endl;
  system("pause");
}
