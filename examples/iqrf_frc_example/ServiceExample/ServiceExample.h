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

#include "PrfThermometer.h"
#include "JsonUtils.h"
#include "IService.h"
#include "ISerializer.h"
#include "IMessaging.h"
#include "IScheduler.h"
#include "TaskQueue.h"
#include <string>
#include <chrono>
#include <vector>
#include <memory>

class IDaemon;


typedef std::basic_string<unsigned char> ustring;

template <typename T>
class ScheduleDpaTask
{
public:
  ScheduleDpaTask() = delete;
  ScheduleDpaTask(const T& dpt, IScheduler* schd)
    :m_scheduler(schd)
    , m_dpa(dpt)
    , m_sync(false)
  {}

  ScheduleDpaTask(const ScheduleDpaTask& other)
    : m_taskHandle(other.m_taskHandle)
    , m_scheduler(other.m_scheduler)
    , m_dpa(other.m_dpa)
    , m_sync((bool)other.m_sync)
  {
  }

  virtual ~ScheduleDpaTask() {};

  bool isSync() const { return m_sync; }
  void setSync(bool val) { m_sync = val; }

  void scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now())
  {
    removeSchedule(clientId);
    m_taskHandle = m_scheduler->scheduleTaskPeriodic(clientId, task, sec, tp);
  }

  bool isScheduled()
  {
    return m_taskHandle != IScheduler::TASK_HANDLE_INVALID;
  }

  void removeSchedule(const std::string& clientId)
  {
    m_scheduler->removeTask(clientId, m_taskHandle);
    m_taskHandle = IScheduler::TASK_HANDLE_INVALID;
  }

  T& getDpa() { return m_dpa; }
private:
  std::atomic_bool m_sync{false};
  IScheduler::TaskHandle m_taskHandle = IScheduler::TASK_HANDLE_INVALID;
  IScheduler* m_scheduler;
  T m_dpa;
};

typedef ScheduleDpaTask<PrfThermometer> PrfThermometerSchd;

class ServiceExample : public IService
{
public:
  ServiceExample() = delete;
  ServiceExample(const std::string& name);
  virtual ~ServiceExample();

  void updateConfiguration(const rapidjson::Value& cfg);

  void setDaemon(IDaemon* daemon) override;
  virtual void setSerializer(ISerializer* serializer) override;
  virtual void setMessaging(IMessaging* messaging) override;
  const std::string& getName() const override {
    return m_name;
  }

  void update(const rapidjson::Value& cfg) override;
  void start() override;
  void stop() override;

private:
  void handleMsgFromMessaging(const ustring& msg);
  void handleTaskFromScheduler(const std::string& task);

  void processFrcFromScheduler(const std::string& task);
  void processPrfThermometerFromTaskQueue(PrfThermometerSchd& pm);
  void processPrfThermometerFromScheduler(const std::string& task);

  bool getFrc() { return m_frcActive; }
  void setFrc(bool val);

  std::string m_name;

  uint16_t m_frcPeriod = 5;
  uint16_t m_sleepPeriod = 60;

  IMessaging* m_messaging;
  IDaemon* m_daemon;
  ISerializer* m_serializer;

  std::vector<PrfThermometerSchd> m_watchedThermometers;
  IScheduler::TaskHandle m_frcHandle;
  bool m_frcActive = false;

  std::unique_ptr<TaskQueue<PrfThermometerSchd*>> m_taskQueue;
};
