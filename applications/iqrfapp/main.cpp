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

TRC_INIT()

int handleMessageFromMq(const ustring& message)
{
  TRC_DBG("Received from MQ: " << std::endl << FORM_HEX(message.data(), message.size()));
  std::string msg((char*)message.data(), message.size());
  std::cout << msg << std::endl;
  return 0;
}

int main(int argc, char** argv)
{
  std::string command;
  bool cmdl = false;

  if (argc == 1) {
    cmdl = true;
  } else if (argc < 4 ) {
    std::cerr << "Usage" << std::endl;
    std::cerr << "  iqrfapp [<perif> <num> <command>]" << std::endl << std::endl;
    std::cerr << "Example" << std::endl;
    std::cerr << "  iqrfapp" << std::endl;
    std::cerr << "  iqrfapp Thermometer 1 READ" << std::endl;
    std::cerr << "  iqrfapp LedG 0 PULSE" << std::endl;
    std::cerr << "  iqrfapp LedR 1 PULSE" << std::endl;
    exit (-1);
  } else {
    std::ostringstream os;
    for (int i = 1; i < argc; i++) {
      os << argv[i] << " ";
    }
    command = os.str();
  }

  std::string cfgFileName("iqrfapp.json");
  std::string localMqName("iqrf-daemon-100");
  std::string remoteMqName("iqrf-daemon-110");

  rapidjson::Document cfg;

  try {
    jutils::parseJsonFile(cfgFileName, cfg);
    jutils::assertIsObject("", cfg);

    localMqName = jutils::getPossibleMemberAs<std::string>("LocalMqName", cfg, localMqName);
    remoteMqName = jutils::getPossibleMemberAs<std::string>("RemoteMqName", cfg, remoteMqName);
  }
  catch (std::logic_error &e) {
    CATCH_EX("Cannot read configuration file: " << PAR(cfgFileName), std::logic_error, e);
  }

  TRC_DBG(PAR(remoteMqName) << PAR(localMqName));

  MqChannel* mqChannel = ant_new MqChannel(remoteMqName, localMqName, 1024);

  //Received messages from IQRF channel are pushed to IQRF MessageQueueChannel
  mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  bool run = true;
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

    //TODO wait timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!cmdl) {
      break;
    }
  }

  delete mqChannel;

  return 0;
}
