#pragma once

#include "TaskQueue.h"
#include "IScheduler.h"
#include <string>
#include <chrono>
#include <map>
#include <memory>

class ScheduleRecord;

//typedef std::basic_string<unsigned char> ustring;

class SchedulerMessaging : public IScheduler
{
public:
  SchedulerMessaging();
  virtual ~SchedulerMessaging();

  void start() override;
  void stop() override;

  void makeCommand(const std::string& clientId, const std::string& command) override;
  void registerMessageHandler(const std::string& clientId, MessageHandlerFunc fun) override;
  void unregisterMessageHandler(const std::string& clientId) override;

private:
  int handleScheduledRecord(const ScheduleRecord& record);

  void addScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
  void addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);
  //void removeScheduleRecord(const ScheduleRecord& record);

  ////////////////////////////////
  TaskQueue<ScheduleRecord>* m_dpaTaskQueue;

  std::map<std::string, MessageHandlerFunc> m_messageHandlers;
  std::mutex m_messageHandlersMutex;

  std::vector<std::shared_ptr<ScheduleRecord>> m_scheduleRecords;
  std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<ScheduleRecord>> m_scheduledTasks;
  bool m_scheduledTaskPushed;
  std::mutex m_scheduledTasksMutex;

  std::thread m_timerThread;
  std::atomic_bool m_runTimerThread;
  std::mutex m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  void timer();

};

class ScheduleRecord {
public:
  ScheduleRecord(const std::string& rec);

  std::chrono::system_clock::time_point getNext(const std::tm& actualTime);
  const std::string& getTask() const { return m_dpaTask; }
  const std::string& getClientId() const { return m_clientId; }

  static std::string asString(const std::chrono::system_clock::time_point& tp);
  static void getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr);
  static std::vector<std::string> preparseMultiRecord(const std::string& rec);

private:
  void parse(const std::string& rec);
  int parseItem(std::string& item, int mnm, int mxm);
  //return true - continue false - finish
  bool compareTimeVal(int& cval, int tval, bool& lw) const;
  std::tm m_tm;
  std::string m_next;
  std::string m_dpaTask;
  std::string m_clientId;
};


