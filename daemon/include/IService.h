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

/// forward declarations
class IDaemon;
class ISerializer;
class IMessaging;

/// \class IService
/// \brief IService interface
class IService
{
public:
  /// \brief Start IService instance
  /// \details
  /// IService implementation starts to process incoming messages
  virtual void start() = 0;

  /// \brief Stop IService instance
  /// \details
  /// IService implementation stops processing incoming messages
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

  /// \brief Set IDaemon instance reference
  /// \param [in] daemon referenced instance
  /// \details
  /// Set IDaemon instance reference to access its interface
  virtual void setDaemon(IDaemon* daemon) = 0;

  /// \brief Set ISerializer instance reference
  /// \param [in] serializer referenced instance
  /// \details
  /// Set ISerializer instance reference to access its interface
  virtual void setSerializer(ISerializer* serializer) = 0;

  /// \brief Set IMessaging instance reference
  /// \param [in] messaging referenced instance
  /// \details
  /// Set IMessaging instance reference to access its interface
  virtual void setMessaging(IMessaging* messaging) = 0;

  inline virtual ~IService() {};
};
