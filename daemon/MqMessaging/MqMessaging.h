/*
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

#pragma once

#include "TaskQueue.h"
#include "IMessaging.h"
#include <string>

class IDaemon;
class MqChannel;

typedef std::basic_string<unsigned char> ustring;

/// \class MqMessaging
/// \brief Interprocess messaging
/// \details
/// Implements IMessaging interface for interprocess communication
///
/// Configurable via its update() method accepting JSON properties:
/// ```json
/// "Properties": {
///   "LocalMqName": "iqrf-daemon-110",    #name of local interprocess connection
///   "RemoteMqName" : "iqrf-daemon-100"   #name of remote interprocess connection
/// }
/// ```
class MqMessaging : public IMessaging
{
public:
  MqMessaging() = delete;
  MqMessaging(const std::string& name);

  virtual ~MqMessaging();

  /// IMessaging overriden methods
  void start() override;
  void stop() override;
  void update(const rapidjson::Value& cfg) override;
  const std::string& getName() const override { return m_name; }
  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;

private:
  int handleMessageFromMq(const ustring& mqMessage);

  MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqMessageQueue;

  std::string m_name;
  std::string m_localMqName;
  std::string m_remoteMqName;

  IMessaging::MessageHandlerFunc m_messageHandlerFunc;
};
