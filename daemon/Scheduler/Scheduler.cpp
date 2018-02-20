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

#include "Scheduler.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <algorithm>

using namespace std::chrono;

Scheduler::Scheduler()
  :m_dpaTaskQueue(nullptr)
{
}

Scheduler::~Scheduler()
{
  TRC_ENTER("");
  if (m_timerThread.joinable()) {
    TRC_DBG("Joining scheduler thread");
    m_timerThread.join();
    TRC_DBG("scheduler thread joined");
  }

  TRC_DBG("Try to destroy: " << PAR(m_dpaTaskQueue->size()));
  delete m_dpaTaskQueue;
  m_dpaTaskQueue = nullptr;
  TRC_LEAVE("");
}

void Scheduler::updateConfiguration(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  jutils::assertIsObject("", cfg);

  std::vector<std::shared_ptr<ScheduleRecord>> tempRecords;

  const auto m = cfg.FindMember("TasksJson");

  if (m == cfg.MemberEnd()) { //old textual form
    std::vector<std::string> records = jutils::getMemberAsVector<std::string>("Tasks", cfg);
    for (auto & it : records) {
      try {
        tempRecords.push_back(std::shared_ptr<ScheduleRecord>(ant_new ScheduleRecord(it)));
      }
      catch (std::exception &e) {
        CATCH_EX("Cought when parsing scheduler table: ", std::exception, e);
      }
    }
  }
  else { //enhanced Json form

    try {
      const auto v = jutils::getMember("TasksJson", cfg);
      if (!v->value.IsArray())
        THROW_EX(std::logic_error, "Expected: Json Array, detected: " << NAME_PAR(name, v->value.GetString()) << NAME_PAR(type, v->value.GetType()));

      for (auto it = v->value.Begin(); it != v->value.End(); ++it) {
        tempRecords.push_back(std::shared_ptr<ScheduleRecord>(ant_new ScheduleRecord(*it)));
      }
    }
    catch (std::exception &e) {
      CATCH_EX("Cought when parsing scheduler table: ", std::exception, e);
    }
  }

  addScheduleRecords(tempRecords);

  TRC_LEAVE("");
}

void Scheduler::start()
{
  TRC_ENTER("");

  m_dpaTaskQueue = ant_new TaskQueue<ScheduleRecord>([&](const ScheduleRecord& record) {
    handleScheduledRecord(record);
  });

  m_scheduledTaskPushed = false;
  m_runTimerThread = true;
  m_timerThread = std::thread(&Scheduler::timer, this);

  TRC_INF("Scheduler started");

  TRC_LEAVE("");
}

void Scheduler::stop()
{
  TRC_ENTER("");
  {
    m_runTimerThread = false;
    std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
    m_scheduledTaskPushed = true;
    m_conditionVariable.notify_one();
  }

  m_dpaTaskQueue->stopQueue();

  TRC_INF("Scheduler stopped");
  TRC_LEAVE("");
}

Scheduler::TaskHandle Scheduler::scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp)
{
  std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(ant_new ScheduleRecord(clientId, task, tp));
  return addScheduleRecord(s);
}

Scheduler::TaskHandle Scheduler::scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
  const std::chrono::system_clock::time_point& tp)
{
  std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(ant_new ScheduleRecord(clientId, task, sec, tp));
  return addScheduleRecord(s);
}

int Scheduler::handleScheduledRecord(const ScheduleRecord& record)
{
  //TRC_DBG("==================================" << std::endl <<
  //  "Scheduled msg: " << std::endl << FORM_HEX(record.getTask().data(), record.getTask().size()));

  {
    std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
    auto found = m_messageHandlers.find(record.getClientId());
    if (found != m_messageHandlers.end()) {
      //TRC_DBG(NAME_PAR(Task, record.getTask()) << " has been passed to: " << NAME_PAR(ClinetId, record.getClientId()));
      found->second(record.getTask());
    }
    else {
      TRC_DBG("Unregistered client: " << PAR(record.getClientId()));
    }
  }

  return 0;
}

Scheduler::TaskHandle Scheduler::addScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record)
{
  system_clock::time_point timePoint;
  std::tm timeStr;
  ScheduleRecord::getTime(timePoint, timeStr);
  TRC_DBG(ScheduleRecord::asString(timePoint));

  //add according time
  system_clock::time_point tp = record->getNext(timePoint, timeStr);
  m_scheduledTasksByTime.insert(std::make_pair(tp, record));

  //add according handle
  while (true) {//get unique handle
    auto result = m_scheduledTasksByHandle.insert(std::make_pair(record->getTaskHandle(), record));
    if (result.second)
      break;
    else
      shuffleDuplicitHandle(*record);
  }

  return record->getTaskHandle();
}

Scheduler::TaskHandle Scheduler::addScheduleRecord(std::shared_ptr<ScheduleRecord>& record)
{
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);

  addScheduleRecordUnlocked(record);

  // notify timer thread
  std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
  m_scheduledTaskPushed = true;
  m_conditionVariable.notify_one();

  return record->getTaskHandle();
}

void Scheduler::addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records)
{
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);

  for (auto & record : records) {
    addScheduleRecordUnlocked(record);
  }

  // notify timer thread
  std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
  m_scheduledTaskPushed = true;
  m_conditionVariable.notify_one();
}

void Scheduler::removeScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record)
{
  Scheduler::TaskHandle handle = record->getTaskHandle();
  for (auto it = m_scheduledTasksByTime.begin(); it != m_scheduledTasksByTime.end(); ) {
    if (it->second->getTaskHandle() == handle)
      it = m_scheduledTasksByTime.erase(it);
    else
      it++;
  }
  m_scheduledTasksByHandle.erase(handle);
}

void Scheduler::removeScheduleRecord(std::shared_ptr<ScheduleRecord>& record)
{
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  removeScheduleRecordUnlocked(record);
}

void Scheduler::removeScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records)
{
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  for (auto & record : records) {
    removeScheduleRecordUnlocked(record);
  }
}

//thread function
void Scheduler::timer()
{
  system_clock::time_point timePoint;
  std::tm timeStr;
  ScheduleRecord::getTime(timePoint, timeStr);
  TRC_DBG(ScheduleRecord::asString(timePoint));

  while (m_runTimerThread) {

    { // wait for something in the m_scheduledTasks;
      std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
      m_conditionVariable.wait_until(lck, timePoint, [&] { return m_scheduledTaskPushed; });
      m_scheduledTaskPushed = false;
    }

    // get actual time
    ScheduleRecord::getTime(timePoint, timeStr);

    // fire all expired tasks
    while (m_runTimerThread) {

      m_scheduledTasksMutex.lock();

      if (m_scheduledTasksByTime.empty()) {
        nextWakeupAndUnlock(timePoint);
        break;
      }

      auto begin = m_scheduledTasksByTime.begin();
      std::shared_ptr<ScheduleRecord> record = begin->second;
      auto diff = begin->first.time_since_epoch().count() - timePoint.time_since_epoch().count();

      if (begin->first < timePoint) {

        // erase fired
        m_scheduledTasksByTime.erase(begin);

        // get and schedule next
        system_clock::time_point nextTimePoint = record->getNext(timePoint, timeStr);
        if (nextTimePoint >= timePoint) {
          m_scheduledTasksByTime.insert(std::make_pair(nextTimePoint, record));
        }
        else {
          //TODO remove from m_scheduledTasksByHandle
        }

        nextWakeupAndUnlock(timePoint);

        if (record->verifyTimePattern(timeStr)) {
          // fire
          //TRC_INF("Task fired at: " << ScheduleRecord::asString(timePoint) << PAR(record->getTask()));
          m_dpaTaskQueue->pushToQueue(*record); //copy record
        }

      }
      else {
        nextWakeupAndUnlock(timePoint);
        break;
      }
    }
  }
}

void Scheduler::nextWakeupAndUnlock(system_clock::time_point& timePoint)
{
  // get next wakeup time
  if (!m_scheduledTasksByTime.empty()) {
    timePoint = m_scheduledTasksByTime.begin()->first;
  }
  else {
    timePoint += seconds(10);
  }
  //TRC_DBG("UNLOCKING MUTEX");
  m_scheduledTasksMutex.unlock();
}

void Scheduler::registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun)
{
  std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
  //TODO check success
  m_messageHandlers.insert(make_pair(clientId, fun));
}

void Scheduler::unregisterMessageHandler(const std::string& clientId)
{
  std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
  m_messageHandlers.erase(clientId);
}

std::vector<std::string> Scheduler::getMyTasks(const std::string& clientId) const
{
  std::vector<std::string> retval;
  // lock and copy
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  for (auto & task : m_scheduledTasksByTime) {
    if (task.second->getClientId() == clientId) {
      retval.push_back(task.second->getTask());
    }
  }
  return retval;
}

std::string Scheduler::getMyTask(const std::string& clientId, const TaskHandle& hndl) const
{
  std::string retval;
  // lock and copy
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  auto found = m_scheduledTasksByHandle.find(hndl);
  if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
    retval = found->second->getTask();
  return retval;
}

void Scheduler::removeAllMyTasks(const std::string& clientId)
{
  // lock and remove
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  for (auto it = m_scheduledTasksByTime.begin(); it != m_scheduledTasksByTime.end(); ) {
    if (it->second->getClientId() == clientId) {
      m_scheduledTasksByHandle.erase(it->second->getTaskHandle());
      it = m_scheduledTasksByTime.erase(it);
    }
    else
      it++;
  }
}

void Scheduler::removeTask(const std::string& clientId, TaskHandle hndl)
{
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  auto found = m_scheduledTasksByHandle.find(hndl);
  if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
    removeScheduleRecordUnlocked(found->second);
}

void Scheduler::removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls)
{
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  for (auto& it : hndls) {
    auto found = m_scheduledTasksByHandle.find(it);
    if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
      removeScheduleRecordUnlocked(found->second);
  }
}

/////////////////////////////////////////
class RandomTaskHandleGenerator {
private:
  RandomTaskHandleGenerator() {
    //init random seed:
    srand(time(NULL));
  }
public:
  static Scheduler::TaskHandle getTaskHandle() {
    static RandomTaskHandleGenerator rt;
    int val = rand();
    return (Scheduler::TaskHandle)(val ? val : val + 1);
  }
};

void ScheduleRecord::init()
{
  m_taskHandle = RandomTaskHandleGenerator::getTaskHandle();
  TRC_DBG("Created: " << PAR(m_taskHandle));
}

//friend of ScheduleRecord
void shuffleDuplicitHandle(ScheduleRecord& rec)
{
  rec.shuffleHandle();
}

void ScheduleRecord::shuffleHandle()
{
  Scheduler::TaskHandle taskHandleOrig = m_taskHandle;
  m_taskHandle = RandomTaskHandleGenerator::getTaskHandle();
  TRC_DBG("Shuffled: " << PAR(m_taskHandle) << PAR(taskHandleOrig));
}

ScheduleRecord::ScheduleRecord(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp)
  : m_exactTime(true)
  , m_clientId(clientId)
  , m_startTime(tp)
{
  init();

  std::time_t tt = system_clock::to_time_t(tp);
  struct std::tm * ptm = std::localtime(&tt);

  m_vsec.push_back(ptm->tm_sec);
  m_vmin.push_back(ptm->tm_min);
  m_vhour.push_back(ptm->tm_hour);
  m_vmday.push_back(ptm->tm_mday);
  m_vmon.push_back(ptm->tm_mon);
  m_vyear.push_back(ptm->tm_year);
  m_vwday.push_back(ptm->tm_wday);
  m_task = task;

}

ScheduleRecord::ScheduleRecord(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
  const std::chrono::system_clock::time_point& tp)
  : m_periodic(true)
  , m_clientId(clientId)
{
  init();

  m_period = sec;
  m_startTime = tp;
  m_task = task;
}

const std::map<std::string, std::string> NICKNAMES = {
  {"@reboot", ""},
  {"@yearly", "0 0 0 0 1 1 *" },
  {"@annually", "0 0 0 0 1 1 *" },
  {"@monthly", "0 0 0 0 1 * *" },
  {"@weekly", "0 0 0 * * * 0" },
  {"@daily", "0 0 0 * * * *" },
  {"@hourly", "0 0 * * * * *" },
  {"@minutely", "0 * * * * * *" }
};

std::string ScheduleRecord::solveNickname(const std::string& timeSpec)
{
  if (timeSpec.empty()) {
    THROW_EX(std::logic_error, "Unexpected empty format:" << PAR(timeSpec));
  }
  if (!timeSpec.empty() && timeSpec[0] == '@') {
    auto found = NICKNAMES.find(timeSpec);
    if (found != NICKNAMES.end()) {
      if (found->second.empty()) { //reboot
        m_exactTime = true;
        m_startTime = std::chrono::system_clock::now();
        //return "" - second.empty()
      }
      return found->second;
    }
    THROW_EX(std::logic_error, "Unexpected format:" << PAR(timeSpec));
  }
  return timeSpec;
}

ScheduleRecord::ScheduleRecord(const std::string& rec)
{
  init();
  
  std::string recStr = solveNickname(rec);
  
  if (m_exactTime)
    return; //exact time is set - no reason to parse next items

  std::istringstream is(recStr);

  std::string sec("*");
  std::string mnt("*");
  std::string hrs("*");
  std::string day("*");
  std::string mon("*");
  std::string yer("*");
  std::string dow("*");

  is >> sec >> mnt >> hrs >> day >> mon >> yer >> dow >> m_clientId;
  std::getline(is, m_task, '|'); // just to get rest of line with spaces

  parseItem(sec, 0, 59, m_vsec);
  parseItem(mnt, 0, 59, m_vmin);
  parseItem(hrs, 0, 23, m_vhour);
  parseItem(day, 1, 31, m_vmday);
  parseItem(mon, 1, 12, m_vmon, -1);
  parseItem(yer, 0, 9000, m_vyear); //TODO maximum?
  parseItem(dow, 0, 6, m_vwday);
}

ScheduleRecord::ScheduleRecord(const rapidjson::Value& rec)
{
  using namespace rapidjson;

  init();

  std::string tmr = jutils::getMemberAs<std::string>("time", rec);

  tmr = solveNickname(tmr);

  if (!m_exactTime) {

    std::istringstream is(tmr);

    std::string sec("*");
    std::string mnt("*");
    std::string hrs("*");
    std::string day("*");
    std::string mon("*");
    std::string yer("*");
    std::string dow("*");

    is >> sec >> mnt >> hrs >> day >> mon >> yer >> dow;

    parseItem(sec, 0, 59, m_vsec);
    parseItem(mnt, 0, 59, m_vmin);
    parseItem(hrs, 0, 23, m_vhour);
    parseItem(day, 1, 31, m_vmday);
    parseItem(mon, 1, 12, m_vmon, -1);
    parseItem(yer, 0, 9000, m_vyear); //TODO maximum?
    parseItem(dow, 0, 6, m_vwday);
  
  }

  m_clientId = jutils::getMemberAs<std::string>("service", rec);
  
  const auto msg = rec.FindMember("message");
  if (msg != rec.MemberEnd()) {
    if (msg->value.IsString()) {
      m_task = msg->value.GetString();
    }
    else {
      if (!msg->value.IsObject()) {
        THROW_EX(std::logic_error, "Expected: Json Object, detected: " << NAME_PAR(name, msg->name.GetString()) << NAME_PAR(type, msg->value.GetType()));
      }

      //back to string :-( TODO optimize to hold parsed structure to save time
      StringBuffer buffer;
      PrettyWriter<StringBuffer> writer(buffer);
      msg->value.Accept(writer);
      m_task = buffer.GetString();

    }
  }

  m_clientId = jutils::getMemberAs<std::string>("service", rec);
}

int ScheduleRecord::parseItem(const std::string& item, int mnm, int mxm, std::vector<int>& vec, int offset)
{
  size_t position;
  int val = 0;

  if (item == "*") {
    val = -1;
    vec.push_back(val);
  }

  else if ((position = item.find('/')) != std::string::npos) {
    if (++position > item.size() - 1)
      THROW_EX(std::logic_error, "Unexpected format");
    int divid = std::stoi(item.substr(position));
    if (divid <= 0)
      THROW_EX(std::logic_error, "Invalid value: " << PAR(divid));

    val = mnm % divid;
    val = val == 0 ? mnm : mnm - val + divid;
    while (val <= mxm) {
      vec.push_back(val + offset);
      val += divid;
    }
    val = mnm;
  }

  else if ((position = item.find(',')) != std::string::npos) {
    position = 0;
    std::string substr = item;
    while (true) {
      val = std::stoi(substr, &position);
      if (val < mnm || val > mxm)
        THROW_EX(std::logic_error, "Invalid value: " << PAR(val));
      vec.push_back(val + offset);

      if (++position > substr.size() - 1)
        break;
      substr = substr.substr(position);
    }
    val = mnm;
  }

  else {
    val = std::stoi(item);
    if (val < mnm || val > mxm) {
      THROW_EX(std::logic_error, "Invalid value: " << PAR(val));
    }
    vec.push_back(val + offset);
  }

  std::sort(vec.begin(), vec.end());
  return val;
}

system_clock::time_point ScheduleRecord::getNext(const std::chrono::system_clock::time_point& actualTimePoint, const std::tm& actualTime)
{
  system_clock::time_point tp;

  if (m_exactTime) {
    return m_startTime;
  }
  else if (!m_periodic) {

    //evaluate remaining seconds
    int asec = actualTime.tm_sec;
    int fsec = asec;
    int dsec = 0;

    //find closest valid sec
    if (m_vsec.size() > 0 && m_vsec[0] < 0) {
      fsec = 0; //seconds * use 0 and period is set to 60 sec by default
    }
    else {
      fsec = m_vsec[0];
      for (int sec : m_vsec) { //try to find greater in vector
        if (sec > asec) {
          fsec = sec;
          break;
        }
      }
    }

    dsec = fsec - asec;
    if (fsec <= asec) {
      dsec += 60;
    }

    tp = actualTimePoint + seconds(dsec);

    //TRC_DBG(
    //  "???????????????????????" << std::endl <<
    //  PAR(dsec) << std::endl <<
    //  NAME_PAR(next, ScheduleRecord::asString(tp)) << std::endl <<
    //  "???????????????????????" << std::endl
    //);

  }
  else {
    if (m_started)
      tp = actualTimePoint + m_period;
    else {
      tp = m_startTime;
      m_started = true;
    }
  }

  return tp;
}

bool ScheduleRecord::verifyTimePattern(const std::tm& actualTime) const
{
  if (!m_periodic && !m_exactTime) {
    if (!verifyTimePattern(actualTime.tm_min, m_vmin)) return false;
    if (!verifyTimePattern(actualTime.tm_hour, m_vhour)) return false;
    if (!verifyTimePattern(actualTime.tm_mday, m_vmday)) return false;
    if (!verifyTimePattern(actualTime.tm_mon, m_vmon)) return false;
    if (!verifyTimePattern(actualTime.tm_year, m_vyear)) return false;
    if (!verifyTimePattern(actualTime.tm_wday, m_vwday)) return false;
  }
  return true;
}

bool ScheduleRecord::verifyTimePattern(int cval, const std::vector<int>& tvalV) const
{
  if (tvalV.size() > 0 && tvalV[0] >= 0) {
    for (int tval : tvalV)
      if (tval == cval)
        return true;
    return false;
  }
  else
    return true;
}


std::string ScheduleRecord::asString(const std::chrono::system_clock::time_point& tp)
{
  // convert to system time:
  std::time_t t = std::chrono::system_clock::to_time_t(tp);
  std::string ts = ctime(&t);   // convert to calendar time
  ts.resize(ts.size() - 1);       // skip trailing newline
  return ts;
}

void ScheduleRecord::getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr)
{
  timePoint = system_clock::now();
  time_t tt;
  tt = system_clock::to_time_t(timePoint);
  std::tm* timeinfo;
  timeinfo = localtime(&tt);
  timeStr = *timeinfo;
}
