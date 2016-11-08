#include "MqChannel.h"
#include "DpaTask.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"
#include <vector>
#include <string>
#include <sstream>

TRC_INIT("");

int handleMessageFromMq(const ustring& message)
{
  TRC_WAR("Received from MQ: " << std::endl << FORM_HEX(message.data(), message.size()));
  std::string msg((char*)message.data(), message.size());
  std::cout << msg << std::endl;
  return 0;
}

int main(int argc, char** argv)
{
  int addrNum = -1;
  std::string device;
  std::string addrStr;
  bool cmdl = false;

  if (argc == 1) {
    cmdl = true;
  }
  else if (argc != 3 ) {
    std::cerr << "Usage" << std::endl;
    std::cerr << "  iqrfapp [temp <num>]" << std::endl << std::endl;
    std::cerr << "Example" << std::endl;
    std::cerr << "  iqrfapp" << std::endl;
    std::cerr << "  iqrfapp temp 1" << std::endl;
    std::cerr << "  iqrfapp pulseG 1" << std::endl;
    std::cerr << "  iqrfapp pulseR 1" << std::endl;
    exit (-1);
  }
  else {
    device = argv[1];
    addrStr = argv[2];
  }

  MqChannel* mqChannel = ant_new MqChannel("iqrf-daemon-110", "iqrf-daemon-100", 1024);

  //Received messages from IQRF channel are pushed to IQRF MessageQueueChannel
  mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  bool run = true;
  while (run)
  {
    if (cmdl) {
      addrNum = -1;
      std::string command;
      std::cout << std::endl << ">> ";

      getline(std::cin, command);

      std::istringstream iscmd(command);
      iscmd >> device >> addrStr;
    }

    std::istringstream is(addrStr);
    is >> addrNum;
    if (addrNum < 0) {
      std::cerr << "invalid address: " << addrStr;
      if (cmdl)
        continue;
      else
        break;
    }

    std::ostringstream os;
    os << device << " " << addrNum;
    std::string msgStr(os.str());
    ustring msg((unsigned char*)msgStr.data(), msgStr.size());

    try {
      mqChannel->sendTo(msg);
    }
    catch (std::exception& e) {
      TRC_DBG("sendTo failed: " << e.what());
      std::cerr << "send failure" << std::endl;
    }

    //TODO wait timeout
    Sleep(200);
    if (!cmdl)
      break;
  }

  delete mqChannel;

  return 0;
}

#if 0
int handleMessageFromMq(const ustring& message)
{
  TRC_WAR("Received from MQ: " << std::endl << FORM_HEX(message.data(), message.size()));
  if (message.size() == 11) {
    //TODO just an example
    DpaThermometer thm(1); //address 1
    DpaMessage dpamsg;
    dpamsg.DataToBuffer(message.data(), message.length());
    thm.parseResponse(dpamsg);
    thm.getTemperature();
    TRC_WAR("Received temperature from: " << NAME_PAR(addr, thm.getAddress()) << NAME_PAR(temperature, thm.getTemperature()));
  }
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
        ustring message = { 0x01, 0x00, 0x0A, 0x00, 0xFF, 0xFF };
        try {
          mqChannel->sendTo(message);
        }
        catch (std::exception& e) {
          TRC_DBG("sendTo failed: " << e.what());
        }
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
#endif
