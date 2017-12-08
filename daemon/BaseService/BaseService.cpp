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
#include "DpaRaw.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

INIT_COMPONENT(IService, BaseService)

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
}

void BaseService::setMessaging(IMessaging* messaging)
{
  m_messaging = messaging;
  m_messaging->registerMessageHandler([&](const ustring& msg) {
    handleMsgFromMessaging(msg);
  });
}

void BaseService::update(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  m_asyncDpaMessage = jutils::getPossibleMemberAs<bool>("AsyncDpaMessage", cfg, m_asyncDpaMessage);
  TRC_LEAVE("");
}

void BaseService::start()
{
  TRC_ENTER("");

  m_daemon->getScheduler()->registerMessageHandler(m_name, [&](const std::string& msg) {
    ustring msgu((unsigned char*)msg.data(), msg.size());
    handleMsgFromMessaging(msgu);
  });

  if (m_asyncDpaMessage) {
    TRC_INF("Set AsyncDpaMessageHandler :" << PAR(m_name));
    m_daemon->registerAsyncMessageHandler(m_name, [&](const DpaMessage& dpaMessage) {
      handleAsyncDpaMessage(dpaMessage);
    });
  }

  TRC_INF("BaseService :" << PAR(m_name) << " started");

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
  TRC_INF(std::endl << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl <<
    "Message to process: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //to encode output message
  std::ostringstream os;

  //get input message
  std::string msgs((const char*)msg.data(), msg.size());
  std::istringstream is(msgs);

  std::unique_ptr<DpaTask> dpaTask;
  std::string command;

  //parse
  bool handled = false;
  std::string ctype;
  std::string lastError = "Unknown ctype";
  for (auto ser : m_serializerVect) {
    ctype = ser->parseCategory(msgs);
    if (ctype == CAT_DPA_STR) {
      dpaTask = ser->parseRequest(msgs);
      if (dpaTask) {
        DpaTransactionTask trans(*dpaTask);
        m_daemon->executeDpaTransaction(trans);
        int result = trans.waitFinish();
        os << dpaTask->encodeResponse(trans.getErrorStr());
        //TODO
        //just stupid hack for test async - remove it
        ///////
        //handleAsyncDpaMessage(dpaTask->getResponse());
        //handleAsyncDpaMessage(dpaTask->getRequest());
        ///////
        handled = true;
      }
      lastError = ser->getLastError();
      break;
    }
    else if (ctype == CAT_CONF_STR) {
      command = ser->parseConfig(msgs);
      if (!command.empty()) {
        std::string response = m_daemon->doCommand(command);
        lastError = ser->getLastError();
        os << ser->encodeConfig(msgs, response);
        handled = true;
      }
      lastError = ser->getLastError();
      break;
    }
  }

  if (!handled) {
    os << "PARSE ERROR: " << PAR(ctype) << PAR(lastError);
  }

  TRC_INF("Response to send: " << std::endl << FORM_HEX(msg.data(), msg.size()) << std::endl <<
    ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl);

  ustring msgu((unsigned char*)os.str().data(), os.str().size());
  m_messaging->sendMessage(msgu);
}

void BaseService::handleAsyncDpaMessage(const DpaMessage& dpaMessage)
{
  TRC_ENTER("");
  std::string sr = m_serializerVect[0]->encodeAsyncAsDpaRaw(dpaMessage);
  TRC_INF(std::endl << "<<<<< ASYNCHRONOUS <<<<<<<<<<<<<<<" << std::endl <<
    "Asynchronous message to send: " << std::endl << FORM_HEX(sr.data(), sr.size()) << std::endl <<
    ">>>>> ASYNCHRONOUS >>>>>>>>>>>>>>>" << std::endl);
  ustring msgu((unsigned char*)sr.data(), sr.size());
  m_messaging->sendMessage(msgu);
  TRC_LEAVE("");
}
