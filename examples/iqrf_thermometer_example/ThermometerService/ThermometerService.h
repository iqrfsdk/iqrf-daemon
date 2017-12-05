/**
 * Copyright 2017 IQRF Tech s.r.o.
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

#include "PrfThermometer.h"
#include "JsonUtils.h"
#include "IService.h"
#include "IMessaging.h"
#include <string>
#include <map>
#include <memory>

class IDaemon;
class IMessaging;
class ISerializer;

class ThermometerService : public IService
{
public:
  ThermometerService() = delete;
  ThermometerService(const std::string& name);
  virtual ~ThermometerService();

  void updateConfiguration(const rapidjson::Value& cfg);

  void setDaemon(IDaemon* daemon) override;
  void setSerializer(ISerializer* serializer) override;
  void setMessaging(IMessaging* messaging) override;
  const std::string& getName() const override {
    return m_name;
  }

  void update(const rapidjson::Value& cfg) override;
  void start() override;
  void stop() override;

private:
  void handleMsgFromMessaging(const IMessaging::ustring& msg);
  void handleTaskFromScheduler(const std::string& task);
  void processThermometersRead();
  void scheduleReading();

  std::string m_name;
  uint16_t m_readPeriod = 60;
  int m_timeout = 1000;

  IMessaging* m_messaging;
  IDaemon* m_daemon;

  struct Val {
    double value = -273.15;
    double valid = false;
  };

  std::map<int,Val> m_thermometers;
  IScheduler::TaskHandle m_schdTaskHandle = IScheduler::TASK_HANDLE_INVALID;
  std::mutex m_mtx;
};
