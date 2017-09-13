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

void helpAndExit()
{
  std::cerr << "Usage" << std::endl;
  std::cerr << "  iqrfapp" << std::endl << std::endl;
  std::cerr << "  iqrfapp help" << std::endl << std::endl;
  std::cerr << "  iqrfapp raw <DpaBytesSeparatedByDots> timeout <ValueInMs>" << std::endl << std::endl;
  std::cerr << "  iqrfapp conf [operational|forwarding|service]" << std::endl << std::endl;
  std::cerr << "  iqrfapp <JsonDpaRequest>" << std::endl << std::endl;
  std::cerr << "Example" << std::endl;
  std::cerr << "  iqrfapp" << std::endl;
  std::cerr << "  iqrfapp help" << std::endl;
  std::cerr << "  iqrfapp raw 01.00.06.03.ff.ff timeout 1000" << std::endl;
  std::cerr << "  iqrfapp conf operational" << std::endl;
  std::cerr << "  iqrfapp \"{\\\"ctype\\\":\\\"dpa\\\",\\\"type\\\":\\\"raw\\\",\\\"request\\\":\\\"01.00.06.03.ff.ff\\\",\\\"timeout\\\":1000}\"" << std::endl;
                  /*"{\"ctype\":\"dpa\",\"type\":\"raw\",\"request\":\"01.00.06.03.ff.ff\",\"timeout\":1000}"*/
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
  std::cout << msg << std::endl;

  exitFlag = true;
  return 0;
}

//just workaround - TODO cmdl params are not properly tested - maybe boost?
int main(int argc, char** argv)
{
  TRC_START("", iqrf::Level::inf, 0);
  std::string command;
  int defaultTimeout = 5000;
  bool cmdl = false;

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
    else {
      int timeout = parseCommand(arg1);
      if (timeout >= 0) {
        defaultTimeout = timeout;
      }
      TRC_DBG("Default timeout: " << PAR(defaultTimeout));
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
      int timeout = -1;
      timeout = std::stoi(arg4);
      if (timeout >= 0) {
        defaultTimeout = timeout;
      }
      TRC_DBG("Default timeout: " << PAR(defaultTimeout));
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

    try {
      mqChannel->sendTo(msgu);
    }
    catch (std::exception& e) {
      TRC_DBG("sendTo failed: " << e.what());
      std::cerr << "send failure" << std::endl;
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
      while (exitFlag)
        ;
    }

    if (!cmdl) {
      break;
    }

    // wait a bit before exit; safety
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

//TODO solve blocking dtor on WIN
#ifndef WIN
  delete mqChannel; 
#endif

  return 0;
}
