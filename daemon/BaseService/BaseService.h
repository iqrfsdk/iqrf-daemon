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
#include "IService.h"
#include "ISerializer.h"
#include "IMessaging.h"
#include "IScheduler.h"
#include <string>
#include <vector>

class IDaemon;

typedef std::basic_string<unsigned char> ustring;


/// \class BaseService
/// \brief Provides basic message handling
/// \details
/// Implements IService interface. It is basic implementation and works just as proxy between
/// IQRF mesh and outside world. It uses IDaemon to communicate via DPA messages with IQRF mesh.
/// It registers to IMessaging to communicate via general messages with outside world.
/// It is possible to set more ISerializer instances for parsing or encoding messages
/// received via IMessaging. It selects appropriate ISerializer instance according incoming messages types.
/// It gets via IDaemon IScheduler to access scheduler methods.
///
/// Configurable via its update() method accepting JSON properties:
/// ```json
/// "Properties": {
///   "AsyncDpaMessage": true  #process asynchronous DPA message
/// }
/// ```
class BaseService : public IService
{
public:
  BaseService() = delete;
  
  /// \brief parametric constructor
  /// \param [in] name unique name of this instance
  BaseService(const std::string& name);

  virtual ~BaseService();

  // IService override methods
  void setDaemon(IDaemon* daemon) override;
  virtual void setSerializer(ISerializer* serializer) override;
  virtual void setMessaging(IMessaging* messaging) override;
  const std::string& getName() const override { return m_name; }
  void update(const rapidjson::Value& cfg) override;
  void start() override;
  void stop() override;

private:
  void handleMsgFromMessaging(const ustring& msg);
  void handleAsyncDpaMessage(const DpaMessage& dpaMessage);

  std::string m_name;
  IMessaging* m_messaging;
  IDaemon* m_daemon;
  std::vector<ISerializer*> m_serializerVect;
  bool m_asyncDpaMessage = false;
};
