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
#include "IqrfLogging.h"
#include "MqMessaging.h"
#include "MqChannel.h"
#include "IDaemon.h"

INIT_COMPONENT(IMessaging, MqMessaging)

const unsigned IQRF_MQ_BUFFER_SIZE = 64*1024;

MqMessaging::MqMessaging(const std::string& name)
  : m_mqChannel(nullptr)
  , m_toMqMessageQueue(nullptr)
  , m_name(name)
  , m_localMqName("iqrf-daemon-110")
  , m_remoteMqName("iqrf-daemon-100")
{
}

MqMessaging::~MqMessaging()
{
}

void MqMessaging::start()
{
  TRC_ENTER("");

  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  TRC_INF("daemon-MQ-protocol started");

  TRC_LEAVE("");
}

void MqMessaging::stop()
{
  TRC_ENTER("");
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  TRC_INF("daemon-MQ-protocol stopped");
  TRC_LEAVE("");
}

void MqMessaging::update(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  m_localMqName = jutils::getPossibleMemberAs<std::string>("LocalMqName", cfg, m_localMqName);
  m_remoteMqName = jutils::getPossibleMemberAs<std::string>("RemoteMqName", cfg, m_remoteMqName);
  TRC_LEAVE("");
}

void MqMessaging::registerMessageHandler(MessageHandlerFunc hndl)
{
  m_messageHandlerFunc = hndl;
}

void MqMessaging::unregisterMessageHandler()
{
  m_messageHandlerFunc = IMessaging::MessageHandlerFunc();
}

void MqMessaging::sendMessage(const ustring& msg)
{
  TRC_DBG(FORM_HEX(msg.data(), msg.size()));
  m_toMqMessageQueue->pushToQueue(msg);
}

int MqMessaging::handleMessageFromMq(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQ: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  if (m_messageHandlerFunc)
    m_messageHandlerFunc(mqMessage);

  return 0;
}
