#pragma once

#include "JsonUtils.h"
#include "PrfPulseMeter.h"
#include "IClient.h"
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

  //void scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp)
  //{
  //  m_taskHandle = m_scheduler->scheduleTaskAt(clientId, task, tp);
  //}

  void scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now())
  {
    removeSchedule(clientId);
    m_taskHandle = m_scheduler->scheduleTaskPeriodic(clientId, task, sec, tp);
  }

  void isScheduled()
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
  std::atomic_bool m_sync = false;
  IScheduler::TaskHandle m_taskHandle = IScheduler::TASK_HANDLE_INVALID;
  IScheduler* m_scheduler;
  T m_dpa;
};

typedef ScheduleDpaTask<PrfPulseMeterJson> PrfPulseMeterSchd;

class ClientServicePm : public IClient
{
public:
  ClientServicePm() = delete;
  ClientServicePm(const std::string& name);
  virtual ~ClientServicePm();

  void updateConfiguration(const rapidjson::Value& cfg);

  void setDaemon(IDaemon* daemon) override;
  virtual void setSerializer(ISerializer* serializer) override;
  virtual void setMessaging(IMessaging* messaging) override;
  const std::string& getClientName() const override {
    return m_name;
  }
  void start() override;
  void stop() override;

private:
  void handleMsgFromMessaging(const ustring& msg);
  void handleTaskFromScheduler(const std::string& task);

  void processFrcFromScheduler(const std::string& task);
  void processPulseMeterFromTaskQueue(PrfPulseMeterSchd& pm);
  void processPulseMeterFromScheduler(const std::string& task);

  bool getFrc() { return m_frcActive; }
  void setFrc(bool val);

  std::string m_name;

  uint16_t m_frcPeriod = 5;
  uint16_t m_sleepPeriod = 60;

  IMessaging* m_messaging;
  IDaemon* m_daemon;
  ISerializer* m_serializer;

  std::vector<PrfPulseMeterSchd> m_watchedPm;
  IScheduler::TaskHandle m_frcHandle;
  bool m_frcActive = false;

  std::unique_ptr<TaskQueue<PrfPulseMeterSchd*>> m_taskQueue;
};
