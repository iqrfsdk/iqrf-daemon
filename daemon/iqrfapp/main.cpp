#include "MqChannel.h"
#include "DpaTask.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"

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
      device = "";
      addrStr = "";

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
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!cmdl)
      break;
  }

  delete mqChannel;

  return 0;
}
