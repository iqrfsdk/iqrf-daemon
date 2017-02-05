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
  typedef long TaskHandle;

  Scheduler();
  virtual ~Scheduler();

  void start() override;
  void stop() override;

  void updateConfiguration(rapidjson::Value& cfg);

  void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) override;
  void unregisterMessageHandler(const std::string& clientId) override;

  std::vector<std::string> getMyTasks(const std::string& clientId) const override;
  std::string getMyTask(const std::string& clientId, const TaskHandle& hndl) const override;

  TaskHandle scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp) override;
  TaskHandle scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp) override;

  void removeAllMyTasks(const std::string& clientId) override;
  void removeTask(const std::string& clientId, TaskHandle hndl) override;
  void removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls) override;

private:
  int handleScheduledRecord(const ScheduleRecord& record);

  TaskHandle addScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record);
  TaskHandle addScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
  void addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);
  
  void removeScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record);
  void removeScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
  void removeScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);

  ////////////////////////////////
  TaskQueue<ScheduleRecord>* m_dpaTaskQueue;

  std::map<std::string, TaskHandlerFunc> m_messageHandlers;
  std::mutex m_messageHandlersMutex;

  // Scheduled tasks by time contains [time_point, ScheduleRecords] pairs.
  // When the time_point is reached, the task is fired and removed. Another pair is added with the next time_point
  // generated from required time matrice
  std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<ScheduleRecord>> m_scheduledTasksByTime;
  bool m_scheduledTaskPushed;
  mutable std::recursive_mutex m_scheduledTasksMutex;

  std::thread m_timerThread;
  std::atomic_bool m_runTimerThread;
  std::mutex m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  void timer();
  void nextWakeupAndUnlock(std::chrono::system_clock::time_point& timePoint);

  std::map<TaskHandle, std::shared_ptr<ScheduleRecord>> m_scheduledTasksByHandle;

};

class ScheduleRecord {
public:
  ScheduleRecord() = delete;
  ScheduleRecord(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp);
  ScheduleRecord(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp);
  ScheduleRecord(const std::string& rec);

  Scheduler::TaskHandle getTaskHandle() const { return m_taskHandle; }
  std::chrono::system_clock::time_point getNext(const std::chrono::system_clock::time_point& actualTimePoint, const std::tm& actualTime);
  const std::string& getTask() const { return m_task; }
  const std::string& getClientId() const { return m_clientId; }

  static std::string asString(const std::chrono::system_clock::time_point& tp);
  static void getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr);
  static std::vector<std::string> preparseMultiRecord(const std::string& rec);

private:
  //Change handle it if duplicit detected by Scheduler
  void shuffleHandle(); //change handle it if duplicit exists
  //The only method can do it
  friend void shuffleDuplicitHandle(ScheduleRecord& rec);
  
  void init();

  void parse(const std::string& rec);
  int parseItem(std::string& item, int mnm, int mxm);
  //return true - continue false - finish
  bool compareTimeVal(int& cval, int tval, bool& lw) const;
  std::tm m_tm = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  //std::string m_next;
  std::string m_task;
  std::string m_clientId;

  //explicit timing
  bool m_periodic = false;
  bool m_started = false;
  std::chrono::seconds m_period;
  std::chrono::system_clock::time_point m_startTime;

  Scheduler::TaskHandle m_taskHandle;
};


