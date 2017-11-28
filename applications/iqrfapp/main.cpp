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

#include "PlatformDep.h"

#include "TaskQueue.h"
#include "MqChannel.h"
#include "PlatformDep.h"
#include "IqrfLogging.h"
#include "JsonUtils.h"
#include "VersionInfo.h"
#include <string>
#include <fstream>
#include <stdexcept>
#include <thread>

class Iqrfapp
{
public:
  Iqrfapp();
  ~Iqrfapp();
  int run(const std::vector<std::string>& params);

private:
  const std::string CMD_HELP = "help";
  const std::string CMD_READO = "readonly";
  const std::string CMD_READA = "readall";
  const std::string CMD_RAW = "raw";
  const std::string CMD_CONF = "conf";
  const std::string CMD_JSON = "json";
  const std::string CMD_EXIT = "exit";
  const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

  int runCmd(const std::vector<std::string>& params);
  void handleMessageReceived(const ustring& message);
  bool waitForMessageReceived(int extraTime = 0);
  void showWaitForMessageReceivedResult(bool res);
  void setTimeout(int timeout);
  void sendParamsAsMessage(const std::vector<std::string>& params);

  void interactiveCmd();
  void helpCmd();
  void readonlyCmd(const std::vector<std::string>& params);
  void readallCmd(const std::vector<std::string>& params);
  void rawCmd(const std::vector<std::string>& params);
  void confCmd(const std::vector<std::string>& params);
  void jsonCmd(const std::vector<std::string>& params);

  std::string m_cfgFileName;
  std::string m_localMqName;
  std::string m_remoteMqName;
  iqrf::Level m_level;
  int m_defaultTimeout = 5000;

  TaskQueue<ustring> *m_msgQueue = nullptr;
  MqChannel* m_mqChannel = nullptr;

  std::mutex m_mutexMessageReceived;
  std::condition_variable m_conditionMessageReceived;
  bool m_flagMessageReceived = false;
  ustring m_messageReceived;
  int m_actualTimeout = 5000;
  int m_extraTime = 1000;
};

TRC_INIT()

//TODO cmdl params are not properly parsed in expected cmdl tools way - maybe boost?
int main(int argc, char** argv)
{
  TRC_START("", iqrf::Level::inf, 0);

  TRC_ENTER("");

  Iqrfapp iqrfapp;

  std::vector<std::string> params;

  for (int i = 1; i < argc; i++)
    params.push_back(argv[i]);

  int retval = iqrfapp.run(params);

  TRC_LEAVE(PAR(retval));
  return retval;
}

Iqrfapp::Iqrfapp()
{
  TRC_ENTER("");

  std::string m_cfgFileName = "iqrfapp.json";
  std::string localMqName = "iqrf-daemon-100";
  std::string remoteMqName = "iqrf-daemon-110";

  std::ifstream f;

  //open in working directory
  f.open(m_cfgFileName);

  if (!f.is_open()) {
    std::string pth = "/etc/iqrf-daemon/";
    m_cfgFileName = pth + m_cfgFileName;

    //open in default path
    f.open(m_cfgFileName);
  }

  if (f.is_open()) {
    TRC_DBG("Reading cfg from: " << PAR(m_cfgFileName));

    rapidjson::Document cfg;
    try {
      jutils::parseJsonFile(m_cfgFileName, cfg);
      jutils::assertIsObject("", cfg);

      m_localMqName = jutils::getPossibleMemberAs<std::string>("LocalMqName", cfg, m_localMqName);
      m_remoteMqName = jutils::getPossibleMemberAs<std::string>("RemoteMqName", cfg, m_remoteMqName);

      int timeout = -1;
      timeout = jutils::getPossibleMemberAs<int>("DefaultTimeout", cfg, timeout);
      if (timeout >= 0) {
        m_defaultTimeout = timeout;
      }

      std::string vl = jutils::getPossibleMemberAs<std::string>("VerbosityLevel", cfg, "dbg");

      if (vl == "err") m_level = iqrf::Level::err;
      else if (vl == "war") m_level = iqrf::Level::war;
      else if (vl == "inf") m_level = iqrf::Level::inf;
      else m_level = iqrf::Level::dbg;

      TRC_START("", m_level, 0);
    }
    catch (std::logic_error &e) {
      CATCH_EX("Cannot JSON parse configuration file: " << PAR(m_cfgFileName), std::logic_error, e);
      TRC_DBG("Using default params");
    }
  }
  else {
    TRC_DBG("Using default params");
  }

  f.close();

  //we work with these params
  TRC_DBG(PAR(remoteMqName) << PAR(localMqName) << PAR(m_defaultTimeout));

  //instantiate incomming message queue
  m_msgQueue = ant_new TaskQueue<ustring>([&](ustring msg) {
    handleMessageReceived(msg);
  });

  //instantiate MQ channel
  m_mqChannel = ant_new MqChannel(remoteMqName, localMqName, IQRF_MQ_BUFFER_SIZE);

  //register message handler for messages received from MQ channel
  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    //msgs are pushed to the queue here
    m_msgQueue->pushToQueue(msg);
    return 0;
  });

  TRC_LEAVE("");
}

Iqrfapp::~Iqrfapp()
{
  delete m_msgQueue;
  //TODO solve blocking dtor on WIN
#ifndef WIN
  delete m_mqChannel;
#endif
}

int Iqrfapp::run(const std::vector<std::string>& params)
{
  TRC_ENTER("");

  int retval = 0;

  int waitCnt = 30;
  while (m_mqChannel->getState() != IChannel::State::Ready) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (--waitCnt < 0)
      break;
  }

  if (m_mqChannel->getState() == IChannel::State::Ready) {

    TRC_DBG("IChannel::State::Ready");

    if (0 == params.size()) {
      interactiveCmd();
    }
    else {
      retval = runCmd(params);
    }
  }
  else {
    std::cout << "cannot initialize reading from iqrf-daemon" << std::endl;
    retval = -1;
  }

  TRC_LEAVE(PAR(retval));
  return retval;
}

int Iqrfapp::runCmd(const std::vector<std::string>& params)
{
  TRC_ENTER("");

  int retval = 0;

  if (0 == params.size()) {
    retval = -1;
  }
  else {
    const std::string& cmd = params[0];

    try {
      if (cmd.empty()) {
        retval = 0;
      }
      else if (cmd == CMD_HELP) {
        helpCmd();
      }
      else if (cmd == CMD_READO) {
        readonlyCmd(params);
      }
      else if (cmd == CMD_READA) {
        readallCmd(params);
      }
      else if (cmd == CMD_RAW) {
        rawCmd(params);
      }
      else if (cmd == CMD_CONF) {
        confCmd(params);
      }
      else if (cmd == CMD_EXIT) {
        retval = -3;
      }
      else if (cmd == CMD_JSON) {
        jsonCmd(params);
      }
      else {
        jsonCmd(params);
      }
    }
    catch (std::logic_error& e) {
      std::cerr << e.what() << std::endl;
      helpCmd();
      retval = -1;
    }
  }
  TRC_LEAVE(PAR(retval));
  return retval;
}

void Iqrfapp::handleMessageReceived(const ustring& message)
{
  TRC_ENTER("");
  TRC_DBG("Received from MQ: " << std::endl << FORM_HEX(message.data(), message.size()));

  {
    std::unique_lock<std::mutex> lck(m_mutexMessageReceived);
    m_messageReceived = message;
    m_flagMessageReceived = true;
    m_conditionMessageReceived.notify_all();
  }

  TRC_LEAVE("");
}

bool Iqrfapp::waitForMessageReceived(int extraTime)
{
  TRC_ENTER("");

  bool retval = false;

  if (m_actualTimeout <= 0) {
    std::unique_lock<std::mutex> lck(m_mutexMessageReceived);
    //wait for message
    m_conditionMessageReceived.wait(lck, [&] { return m_flagMessageReceived; }); //lock is released in wait
    //lock is reacquired here
    m_flagMessageReceived = false;
    retval = true;
  }
  else {
    std::unique_lock<std::mutex> lck(m_mutexMessageReceived);
    //wait for message
    //add extraTime to extend waiting time to allow daemon process timeout response (otherwise it end earlier)
    if (m_conditionMessageReceived.wait_for(lck, std::chrono::milliseconds(m_actualTimeout + extraTime), [&] { return m_flagMessageReceived; })) {
      //finished waiting
      m_flagMessageReceived = false;
      retval = true;
    }
    else {
      //timeout
      m_flagMessageReceived = false;
      retval = false;
    }
  }

  TRC_LEAVE(PAR(retval));
  return retval;
}

void Iqrfapp::showWaitForMessageReceivedResult(bool res)
{
  if (res) {
    std::string msg((char*)m_messageReceived.data(), m_messageReceived.size());
    std::cout << "Received: " << msg << std::endl;
  }
  else {
    std::cout << "Timeout" << std::endl;
  }
}

void Iqrfapp::setTimeout(int timeout)
{
  TRC_ENTER("");
  if (timeout >= 0) {
    m_actualTimeout = timeout;
    TRC_DBG("Set: " PAR(m_actualTimeout));
  }
  else {
    m_actualTimeout = m_defaultTimeout;
    TRC_WAR(PAR(timeout) << "is invalid keep " << PAR(m_defaultTimeout));

  }
  TRC_LEAVE("");
}

void Iqrfapp::sendParamsAsMessage(const std::vector<std::string>& params)
{
  TRC_ENTER("");
  std::ostringstream os;
  for (const auto & par : params)
    os << par << " ";

  std::string msg = os.str();
  ustring msgu((unsigned char*)msg.data(), msg.size());

  try {
    m_mqChannel->sendTo(msgu);
  }
  catch (std::exception& e) {
    TRC_DBG("SendTo failed: " << e.what());
    std::cerr << "Send failure: " << e.what() << std::endl;
  }
  TRC_LEAVE("");
}

void Iqrfapp::interactiveCmd()
{
  TRC_ENTER("");
  while (true)
  {
    std::string command;
    std::cout << std::endl << ">> ";
    getline(std::cin, command);

    //parse command
    std::vector<std::string> params;
    std::istringstream is(command);
    std::string par;
    while (!is.eof()) {
      is >> par;
      params.push_back(par);
    }

    int retval = runCmd(params);
    if (-3 == retval)
      break;
  }

  TRC_LEAVE("");
}

void Iqrfapp::helpCmd()
{
  std::cerr << "Usage" << std::endl;
  std::cerr << "  iqrfapp" << std::endl;
  std::cerr << "  iqrfapp help" << std::endl;
  std::cerr << "  iqrfapp readonly [timeout <ValueInMs>]" << std::endl;
  std::cerr << "  iqrfapp readall" << std::endl;
  std::cerr << "  iqrfapp raw <DpaBytesSeparatedByDots> [timeout <ValueInMs>]" << std::endl;
  std::cerr << "  iqrfapp conf operational|forwarding|service" << std::endl;
  std::cerr << "  iqrfapp [json] <JsonDpaRequest>" << std::endl;
  std::cerr << "Examples" << std::endl;
  std::cerr << "  iqrfapp" << std::endl;
  std::cerr << "  iqrfapp help" << std::endl;
  std::cerr << "  iqrfapp raw 01.00.06.03.ff.ff timeout 1000" << std::endl;
  std::cerr << "  iqrfapp conf operational" << std::endl;
  std::cerr << "  iqrfapp \"{\\\"ctype\\\":\\\"dpa\\\",\\\"type\\\":\\\"raw\\\",\\\"timeout\\\":1000,\\\"request\\\":\\\"01.00.06.03.ff.ff\\\"}\"" << std::endl;
  std::cerr << "Tips" << std::endl;
  std::cerr << "  Adjust default timeout in iqrfapp.json to the needs of the application using iqrfapp tool or define timeout as cmd param." << std::endl;
  std::cerr << "  Default timeout in iqrfapp.json is 5000 ms." << std::endl;
}

void Iqrfapp::readonlyCmd(const std::vector<std::string>& params)
{
  TRC_ENTER("");

  if (params.size() != 1 && params.size() != 3) {
    throw std::logic_error("expected 1 or 3 parameters");
  }

  int timeout = m_defaultTimeout;

  if (params.size() == 3) {
    if (params[1] != "timeout") {
      throw std::logic_error("expected 2nd parameter: timeout");
    }

    try {
      timeout = std::stoi(params[2]);
    }
    catch (std::invalid_argument&) {
      throw std::logic_error("expected 3rd parameter <millis> representing integer value");
    }
  }

  setTimeout(timeout);
  std::cout << "Listening for asynchronous messages: " << NAME_PAR(timeout, m_actualTimeout) << std::endl;
  bool res = waitForMessageReceived();
  showWaitForMessageReceivedResult(res);

  TRC_LEAVE("");
}

void Iqrfapp::readallCmd(const std::vector<std::string>& params)
{
  TRC_ENTER("");

  if (params.size() != 1) {
    throw std::logic_error("expected 1 parameters");
  }

  //TODO break by CTRL-C
  while (true) {
    std::cout << "Listening for asynchronous messages (stop by CTRL-C): " << std::endl;
    setTimeout(0);
    bool res = waitForMessageReceived();
    showWaitForMessageReceivedResult(res);
  }

  TRC_LEAVE("");
}

void Iqrfapp::rawCmd(const std::vector<std::string>& params)
{
  TRC_ENTER("");
  if (params.size() != 2 && params.size() != 4) {
    throw std::logic_error("expected 2 or 4 parameters");
  }

  int timeout = m_defaultTimeout;

  if (params.size() == 4) {
    if (params[2] != "timeout") {
      throw std::logic_error("expected 3rd parameter: timeout");
    }

    try {
      timeout = std::stoi(params[3]);
    }
    catch (std::invalid_argument&) {
      throw std::logic_error("expected 4th parameter <millis> representing integer value");
    }
  }

  setTimeout(timeout);
  sendParamsAsMessage(params);

  bool res = waitForMessageReceived(m_extraTime);
  showWaitForMessageReceivedResult(res);

  TRC_LEAVE("");
}

void Iqrfapp::confCmd(const std::vector<std::string>& params)
{
  TRC_ENTER("");
  if (params.size() != 2) {
    throw std::logic_error("expected 2 parameters");
  }

  const std::string par2 = params[1];
  if (par2 != "operational" && par2 != "forwarding" && par2 != "service") {
    throw std::logic_error("expected 3rd parameter: [operational | forwarding | service]");
  }
  else {
  }

  sendParamsAsMessage(params);

  bool res = waitForMessageReceived(m_extraTime);
  showWaitForMessageReceivedResult(res);

  TRC_LEAVE("");
}

void Iqrfapp::jsonCmd(const std::vector<std::string>& params)
{
  TRC_ENTER("");
  std::string json;
  if (params.size() == 2) {
    json = params[1]; //possible format: json <JSON>
  }
  else if (params.size() == 1) {
    json = params[0]; //possible format: <JSON>
  }
  else {
    throw std::logic_error("expected 1 or 2 parameters");
  }

  //parse JSON
  rapidjson::Document doc;
  int timeout = m_defaultTimeout;

  try {
    jutils::parseString(json, doc);
    jutils::assertIsObject("", doc);
    timeout = jutils::getPossibleMemberAs<int>("timeout", doc, timeout); //optional
  }
  catch (std::exception &e) {
    std::string errstr = "expected JSON format: ";
    errstr += e.what();
    throw std::logic_error(errstr);
  }

  setTimeout(timeout);
  sendParamsAsMessage(params);

  bool res = waitForMessageReceived(m_extraTime);
  showWaitForMessageReceivedResult(res);

  TRC_LEAVE("");
}
