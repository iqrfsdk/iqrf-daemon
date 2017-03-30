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

#pragma once

#include "JsonUtils.h"
#include "IMessaging.h"
#include <string>

class Impl;

typedef std::basic_string<unsigned char> ustring;

class MqttMessaging : public IMessaging
{
public:
  MqttMessaging() = delete;
  MqttMessaging(const std::string& name);

  virtual ~MqttMessaging();

  // component
  void start() override;
  void stop() override;
  void update(const rapidjson::Value& cfg) override;
  const std::string& getName() const override;

  // interface
  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;

private:
  Impl* m_impl;
};

class MqttChannelException : public std::exception {
public:
  MqttChannelException(const std::string& cause)
    :m_cause(cause)
  {}

  //TODO ?
#ifndef WIN32
  virtual const char* what() const noexcept(true)
#else
  virtual const char* what() const
#endif
  {
    return m_cause.c_str();
  }

  virtual ~MqttChannelException()
  {}

protected:
  std::string m_cause;
};
