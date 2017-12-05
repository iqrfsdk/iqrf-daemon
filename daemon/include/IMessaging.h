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

#include "JsonUtils.h"
#include <string>
#include <functional>

class IDaemon;

/// \class IMessaging
/// \brief IMessaging interface
/// \details
/// Provides interface for sending/receiving message from/to general communication interface
class IMessaging
{
public:
  /// \brief Start Messaging instance
  /// \details
  /// Messaging implementation starts to listen incoming messages an is ready to send
  virtual void start() = 0;

  /// \brief Stop Messaging instance
  /// \details
  /// Messaging implementation stops its job
  virtual void stop() = 0;

  /// \brief Update configuration
  /// \param [in] cfg configuration data
  /// \details
  /// Configuration data are taken from passed cfg and the instance is configured accordingly
  virtual void update(const rapidjson::Value& cfg) = 0;

  /// \brief Get name of the instance
  /// \return The instance name
  /// \details
  /// Returns unique name of the instance
  virtual const std::string& getName() const = 0;

  typedef std::basic_string<unsigned char> ustring;
  /// Incoming message handler functional type
  typedef std::function<void(const ustring&)> MessageHandlerFunc;

  /// \brief Register message handler
  /// \param [in] hndl registering handler function
  /// \details
  /// Whenever a message is received it is passed to the handler function. It is possible to register 
  /// just one handler
  virtual void registerMessageHandler(MessageHandlerFunc hndl) = 0;

  /// \brief Unregister message handler
  /// \details
  /// If the handler is not required anymore, it is possible to unregister via this method.
  virtual void unregisterMessageHandler() = 0;

  /// \brief send message
  /// \param [in] msg message to be sent 
  /// \details
  /// The message is send outside
  virtual void sendMessage(const ustring& msg) = 0;

  inline virtual ~IMessaging() {};
};
