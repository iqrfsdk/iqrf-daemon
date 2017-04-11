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

#include "LaunchUtils.h"
#include "BaseService.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

INIT_COMPONENT(IClient, BaseService)

BaseService::BaseService(const std::string & name)
  :m_name(name)
  , m_messaging(nullptr)
  , m_daemon(nullptr)
{
}

BaseService::~BaseService()
{
}

void BaseService::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
}

void BaseService::setSerializer(ISerializer* serializer)
{
  m_serializerVect.push_back(serializer);
  m_messaging->registerMessageHandler([&](const ustring& msg) {
    handleMsgFromMessaging(msg);
  });
}

void BaseService::setMessaging(IMessaging* messaging)
{
  m_messaging = messaging;
}

void BaseService::update(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  TRC_LEAVE("");
}

void BaseService::start()
{
  TRC_ENTER("");

  m_daemon->getScheduler()->registerMessageHandler(m_name, [&](const std::string& msg) {
    ustring msgu((unsigned char*)msg.data(), msg.size());
    handleMsgFromMessaging(msgu);
  });

  TRC_INF("ClientServicePlain :" << PAR(m_name) << " started");

  TRC_LEAVE("");
}

void BaseService::stop()
{
  TRC_ENTER("");

  m_daemon->getScheduler()->unregisterMessageHandler(m_name);

  TRC_INF("BaseService :" << PAR(m_name) << " stopped");
  TRC_LEAVE("");
}

void BaseService::handleMsgFromMessaging(const ustring& msg)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MESSAGING: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //to encode output message
  std::ostringstream os;

  //get input message
  std::string msgs((const char*)msg.data(), msg.size());
  std::istringstream is(msgs);

  std::unique_ptr<DpaTask> dpaTask;
  std::string command;

  //parse
  for (auto ser : m_serializerVect) {
    std::string cat = ser->parseCategory(msgs);
    if (cat == CAT_DPA_STR) {
      dpaTask = ser->parseRequest(msgs);
      if (dpaTask) {
        break;
      }
    }
    else if (cat == CAT_CONF_STR) {
      command = ser->parseConfig(msgs);
      if (!command.empty()) {
        break;
      }
    }
  }

  // process parse result
  if (dpaTask) {
    DpaTransactionTask trans(*dpaTask);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();
    os << dpaTask->encodeResponse(trans.getErrorStr());
  }
  else if (!command.empty()) {
    os << m_daemon->doCommand(command);
  }
  else {
    //os << m_serializer->getLastError();
    os << "PARSE ERROR";
  }

  ustring msgu((unsigned char*)os.str().data(), os.str().size());
  m_messaging->sendMessage(msgu);
}
