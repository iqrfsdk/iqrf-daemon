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
#include <string>
#include <functional>

class IDaemon;

class IMessaging
{
public:
  // component
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void update(const rapidjson::Value& cfg) = 0;
  virtual const std::string& getName() const = 0;

  // references
  //virtual void setDaemon(IDaemon* daemon) = 0;

  // interface
  typedef std::basic_string<unsigned char> ustring;
  typedef std::function<void(const ustring&)> MessageHandlerFunc;
  virtual void registerMessageHandler(MessageHandlerFunc hndl) = 0;
  virtual void unregisterMessageHandler() = 0;
  virtual void sendMessage(const ustring& msg) = 0;

  inline virtual ~IMessaging() {};
};
