#pragma once

#include "SimpleSerializer.h"
#include "JsonSerializer.h"
#include "TaskQueue.h"
#include "IMessaging.h"
#include "IScheduler.h"
#include <string>
#include <chrono>
#include <map>
#include <memory>

class IDaemon;
class MqChannel;
class ScheduleRecord;

typedef std::basic_string<unsigned char> ustring;

class SchedulerMessaging : public IMessaging, public IScheduler
{
public:
  SchedulerMessaging();
  virtual ~SchedulerMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

  virtual void makeCommand(const std::string& clientId, const std::string& command);
  virtual void registerResponseHandler(const std::string& clientId, ResponseHandlerFunc fun);
  virtual void unregisterResponseHandler(const std::string& clientId);

private:
  //void sendMessageToMq(const std::string& message);
  int handleScheduledRecord(const ScheduleRecord& record);

  void addScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
  void addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);
  //void removeScheduleRecord(const ScheduleRecord& record);

  IDaemon* m_daemon;
  //MqChannel* m_mqChannel;
  //TaskQueue<ustring>* m_toMqMessageQueue;

  //std::string m_localMqName;
  //std::string m_remoteMqName;
  
  ////////////////////////////////
  TaskQueue<ScheduleRecord>* m_dpaTaskQueue;

  std::map<std::string, ResponseHandlerFunc> m_responseHandlers;
  std::mutex m_responseHandlersMutex;

  std::vector<std::shared_ptr<ScheduleRecord>> m_scheduleRecords;
  std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<ScheduleRecord>> m_scheduledTasks;
  bool m_scheduledTaskPushed;
  std::mutex m_scheduledTasksMutex;

  std::thread m_timerThread;
  std::atomic_bool m_runTimerThread;
  std::mutex m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  void timer();

  DpaTaskSimpleSerializerFactory m_simpleFactory;
  DpaTaskJsonSerializerFactory m_jsonFactory;
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


