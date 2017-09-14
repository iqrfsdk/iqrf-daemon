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

#include "MqChannel.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"
#include "JsonUtils.h"
#include "VersionInfo.h"
#include <fstream>

TRC_INIT()

bool exitFlag = false;
bool sentFlag = false;

void helpAndExit()
{
  std::cerr << "Usage" << std::endl;
  std::cerr << "  iqrfapp" << std::endl;
  std::cerr << "  iqrfapp help" << std::endl;
  std::cerr << "  iqrfapp readonly" << std::endl;
  std::cerr << "  iqrfapp raw <DpaBytesSeparatedByDots> timeout <ValueInMs>" << std::endl;
  std::cerr << "  iqrfapp conf [operational|forwarding|service]" << std::endl;
  std::cerr << "  iqrfapp <JsonDpaRequest>" << std::endl;
  std::cerr << "Examples" << std::endl;
  std::cerr << "  iqrfapp" << std::endl;
  std::cerr << "  iqrfapp help" << std::endl;
  std::cerr << "  iqrfapp raw 01.00.06.03.ff.ff timeout 1000" << std::endl;
  std::cerr << "  iqrfapp conf operational" << std::endl;
  std::cerr << "  iqrfapp \"{\\\"ctype\\\":\\\"dpa\\\",\\\"type\\\":\\\"raw\\\",\\\"timeout\\\":1000,\\\"request\\\":\\\"01.00.06.03.ff.ff\\\"}\"" << std::endl;
                  /*"{\"ctype\":\"dpa\",\"type\":\"raw\",\"timeout\":1000,\"request\":\"01.00.06.03.ff.ff\"}"*/
  std::cerr << "Tips" << std::endl;
  std::cerr << "  Adjust default timeout in iqrfapp.json to the needs of the application using iqrfapp tool or define timeout as cmd param." << std::endl;
  std::cerr << "  Default timeout in iqrfapp.json is 5000 ms." << std::endl;
  exit(-1);
}

int parseCommand(std::string cmd) {

  rapidjson::Document doc;
  int timeout = -1;

  try {
    jutils::parseString(cmd, doc);
    jutils::assertIsObject("", doc);
    timeout = jutils::getMemberAs<int>("timeout", doc);
  }
  catch (std::exception &e) {
    CATCH_EX("Cannot parse command: ", std::logic_error, e);
  }

  return timeout;
}

int handleMessageFromMq(const ustring& message)
{
  TRC_DBG("Received from MQ: " << std::endl << FORM_HEX(message.data(), message.size()));
  std::string msg((char*)message.data(), message.size());

  if (sentFlag) {
    std::cout << msg << std::endl;
    exitFlag = true;
    sentFlag = false;
  }

  return 0;
}

//just workaround - TODO cmdl params are not properly tested - maybe boost?
int main(int argc, char** argv)
{
  TRC_START("", iqrf::Level::inf, 0);
  std::string command;
  std::string rawTimeout;

  bool cmdl = false;
  bool json = false;
  bool raw = false;
  bool readonly = false;

  int defaultTimeout = 5000;

  switch (argc) {
  case 1:
  {
    cmdl = true;
    break;
  }
  case 2:
  {
    std::string arg1 = argv[1];
    if (arg1 == "help") {
      helpAndExit();
    }
    else if (arg1 == "readonly") {
      readonly = true;
    }
    else {
      json = true;
    }
    break;
  }
  case 3:
  {
    std::string arg1 = argv[1];
    std::string arg2 = argv[2];
    if (arg1 == "conf" && arg2 != "operational" && arg2 != "forwarding" && arg2 != "service") {
      helpAndExit();
    }
    if (arg1 != "raw") {
      helpAndExit();
    }
    break;
  }
  // raw with timeout
  case 5:
  {
    std::string arg1 = argv[1];
    std::string arg2 = argv[2];
    std::string arg3 = argv[3];
    std::string arg4 = argv[4];

    if (arg1 != "raw" && arg3 != "timeout") {
      helpAndExit();
    }
    else {
      raw = true;
      rawTimeout = arg4;
    }
    break;
  }
  default:; //just continue and try
  }

  std::ostringstream os;
  for (int i = 1; i < argc; i++)
    os << argv[i] << " ";
  command = os.str();

  std::string cfgFileName("iqrfapp.json");
  std::string localMqName("iqrf-daemon-100");
  std::string remoteMqName("iqrf-daemon-110");
  std::ifstream f;

  f.open(cfgFileName);

  if (!f.is_open()) {
    std::string pth = "/etc/iqrf-daemon/";
    cfgFileName = pth + cfgFileName;
    f.open(cfgFileName);
  }

  if (f.is_open()) {
    TRC_DBG("Reading cfg from: " << PAR(cfgFileName));

    rapidjson::Document cfg;
    try {
      jutils::parseJsonFile(cfgFileName, cfg);
      jutils::assertIsObject("", cfg);

      localMqName = jutils::getPossibleMemberAs<std::string>("LocalMqName", cfg, localMqName);
      remoteMqName = jutils::getPossibleMemberAs<std::string>("RemoteMqName", cfg, remoteMqName);

      int timeout = -1;
      timeout = jutils::getPossibleMemberAs<int>("DefaultTimeout", cfg, timeout);
      if (timeout >= 0) {
        defaultTimeout = timeout;
      }
    }
    catch (std::logic_error &e) {
      CATCH_EX("Cannot read configuration file: " << PAR(cfgFileName), std::logic_error, e);
      TRC_DBG("Using default params");
    }
  }
  else {
    TRC_DBG("Using default params");
  }

  if (json) {
    int timeout = parseCommand(command);
    if (timeout >= 0) {
      defaultTimeout = timeout;
    }
    TRC_DBG("New timeout: " << PAR(defaultTimeout));
  }

  if (raw) {
    int timeout = -1;
    timeout = std::stoi(rawTimeout);
    if (timeout >= 0) {
      defaultTimeout = timeout;
    }
    TRC_DBG("New timeout: " << PAR(defaultTimeout));
  }

  TRC_DBG(PAR(remoteMqName) << PAR(localMqName));

  MqChannel* mqChannel = ant_new MqChannel(remoteMqName, localMqName, 1024);

  // received messages from IQRF channel are pushed to IQRF MessageQueueChannel
  mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  bool run = true;

  if (cmdl) {
    std::cout << std::endl << "iqrfapp " << PAR(IQRFAPP_VERSION) << PAR(BUILD_TIMESTAMP) << std::endl;
  }

  while (run)
  {
    if (cmdl) {
      std::cout << std::endl << ">> ";
      getline(std::cin, command);
    }

    ustring msgu((unsigned char*)command.data(), command.size());

    if(!readonly) {
      try {
        mqChannel->sendTo(msgu);
        sentFlag = true;
      }
      catch (std::exception& e) {
        TRC_DBG("SendTo failed: " << e.what());
        std::cerr << "Send failure" << std::endl;
      }
    }
    else {
      std::cout << "Listening for asynchronous messages, timeout: " << defaultTimeout << std::endl;
    }

    if (defaultTimeout != 0) {
      for (int i = 0; i < defaultTimeout; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // exit when we receive response
        if (exitFlag) break;
      }
    }
    // defaultTimeout == 0 means waiting until there is the response
    else {
      while (!exitFlag)
        ;
    }

    if (!cmdl) {
      break;
    }
  }

//TODO solve blocking dtor on WIN
#ifndef WIN
  delete mqChannel;
#endif

  return 0;
}
