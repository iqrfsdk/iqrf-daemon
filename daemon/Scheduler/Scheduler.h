#pragma once

#include "JsonUtils.h"
#include "TaskQueue.h"
#include "IScheduler.h"
#include <string>
#include <chrono>
#include <map>
#include <memory>

class ScheduleRecord;

class Scheduler : public IScheduler
{
public:
  Scheduler();
  virtual ~Scheduler();

  void start() override;
  void stop() override;

  void updateConfiguration(rapidjson::Value& cfg);

  void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) override;
  void unregisterMessageHandler(const std::string& clientId) override;

  std::vector<std::string> getMyTasks(const std::string& clientId) override;
  void removeAllMyTasks(const std::string& clientId) override;

  ///////////
  typedef uint32_t TaskHandle;
  TaskHandle scheduleTaskAt(const std::string& task, const std::chrono::system_clock::time_point& tp);
  TaskHandle scheduleTaskPeriodic(const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp);
  void removeTask(TaskHandle hndl);

private:
  int handleScheduledRecord(const ScheduleRecord& record);

  TaskHandle addScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
  void addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);
  //void removeScheduleRecord(const ScheduleRecord& record);

  ////////////////////////////////
  TaskQueue<ScheduleRecord>* m_dpaTaskQueue;

  std::map<std::string, TaskHandlerFunc> m_messageHandlers;
  std::mutex m_messageHandlersMutex;

  // Scheduled tasks by time contains [time_point, ScheduleRecords] pairs.
  // When the time_point is reached, the task is fired and removed. Another pair is added with the next time_point
  // generated from required time matrice
  std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<ScheduleRecord>> m_scheduledTasksByTime;
  bool m_scheduledTaskPushed;
  std::mutex m_scheduledTasksMutex;

  std::thread m_timerThread;
  std::atomic_bool m_runTimerThread;
  std::mutex m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  void timer();

  std::map<TaskHandle, std::shared_ptr<ScheduleRecord>> m_scheduledTasksByHandle;

};

class ScheduleRecord {
public:
  ScheduleRecord(const std::string& task, const std::chrono::system_clock::time_point& tp);
  ScheduleRecord(const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now());

  ScheduleRecord(const std::string& rec);

  std::chrono::system_clock::time_point getNext(const std::tm& actualTime);
  const std::string& getTask() const { return m_task; }
  const std::string& getClientId() const { return m_clientId; }

  static std::string asString(const std::chrono::system_clock::time_point& tp);
  static void getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr);
  static std::vector<std::string> preparseMultiRecord(const std::string& rec);

private:
  void parse(const std::string& rec);
  int parseItem(std::string& item, int mnm, int mxm);
  //return true - continue false - finish
  bool compareTimeVal(int& cval, int tval, bool& lw) const;
  std::tm m_tm = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  std::string m_next;
  std::string m_task;
  std::string m_clientId;

  //explicit timing
  bool m_explicitTiming = false;
  std::chrono::seconds m_period;
  std::chrono::system_clock::time_point m_timePoint;
};


