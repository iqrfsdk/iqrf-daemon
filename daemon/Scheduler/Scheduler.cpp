#include "Scheduler.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"

using namespace std::chrono;

Scheduler::Scheduler()
  :m_dpaTaskQueue(nullptr)
{
}

Scheduler::~Scheduler()
{
}

void Scheduler::updateConfiguration(rapidjson::Value& cfg)
{
  TRC_ENTER("");
  jutils::assertIsObject("", cfg);

  std::vector<std::string> records = jutils::getMemberAsVector<std::string>("Tasks", cfg);

  std::vector<std::shared_ptr<ScheduleRecord>> tempRecords;
  int i = 0;
  for (auto & it : records) {
    tempRecords.push_back(std::shared_ptr<ScheduleRecord>(ant_new ScheduleRecord(it)));
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

  if (m_timerThread.joinable())
    m_timerThread.join();

  delete m_dpaTaskQueue;
  m_dpaTaskQueue = nullptr;

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
  TRC_DBG("==================================" << std::endl <<
    "Scheduled msg: " << std::endl << FORM_HEX(record.getTask().data(), record.getTask().size()));

  {
    std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
    auto found = m_messageHandlers.find(record.getClientId());
    if (found != m_messageHandlers.end()) {
      TRC_DBG(NAME_PAR(Response, record.getTask()) << " has been passed to: " << NAME_PAR(ClinetId, record.getClientId()));
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
  for (auto it = m_scheduledTasksByTime.begin(); it != m_scheduledTasksByTime.end(); it++) {
    if (it->second->getTaskHandle() == handle)
      it = m_scheduledTasksByTime.erase(it);
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
      m_conditionVariable.wait_until(lck, timePoint, [&]{ return m_scheduledTaskPushed; });
      m_scheduledTaskPushed = false;
    }

    // get actual time
    ScheduleRecord::getTime(timePoint, timeStr);

    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);

    // fire all expired tasks
    while (!m_scheduledTasksByTime.empty()) {

      auto begin = m_scheduledTasksByTime.begin();
      std::shared_ptr<ScheduleRecord> record = begin->second;

      if (begin->first < timePoint) {
        
        // fire
    	TRC_INF("Task fired at: " << ScheduleRecord::asString(timePoint));
        int retval = m_dpaTaskQueue->pushToQueue(*record); //copy record
        TRC_DBG("Returned from pushToQueue():" << PAR(retval));

        // erase fired
        m_scheduledTasksByTime.erase(begin);

        // get and schedule next
        system_clock::time_point nextTimePoint = record->getNext(timePoint, timeStr);
        if (nextTimePoint >= timePoint) {
          m_scheduledTasksByTime.insert(std::make_pair(nextTimePoint, record));
        }
      }
      else {
        break;
      }
    }

    // get next wakeup time
    if (!m_scheduledTasksByTime.empty()) {
      timePoint = m_scheduledTasksByTime.begin()->first;
    }
    else {
      timePoint += duration<int>(10);
    }

  }
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
    return (Scheduler::TaskHandle)rand();
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
  : m_clientId(clientId)
{
  init();

  std::time_t tt = system_clock::to_time_t(tp);
  struct std::tm * ptm = std::localtime(&tt);

  m_tm.tm_sec = ptm->tm_sec;
  m_tm.tm_min = ptm->tm_min;
  m_tm.tm_hour = ptm->tm_hour;
  m_tm.tm_mday = ptm->tm_mday;
  m_tm.tm_mon = ptm->tm_mon;
  m_tm.tm_year = ptm->tm_year;
  m_tm.tm_wday = ptm->tm_wday;
  m_tm.tm_yday = ptm->tm_yday;
  m_tm.tm_isdst = ptm->tm_isdst;

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

  // split into days, hours, minutes, seconds
  //typedef duration<int, std::ratio<60 * 60 * 24>> days;
  //days    dd = duration_cast<days> (sec);
  //hours   hh = duration_cast<hours>(sec % days(1));
  //minutes mm = duration_cast<minutes>(sec % chrono::hours(1));
  //seconds ss = sec % chrono::minutes(1);
}

ScheduleRecord::ScheduleRecord(const std::string& rec)
{
  init();

  parse(rec);
}

std::vector<std::string> ScheduleRecord::preparseMultiRecord(const std::string& rec)
{
  std::istringstream is(rec);

  std::string sec("*");
  std::string mnt("*");
  std::string hrs("*");
  std::string day("*");
  std::string mon("*");
  std::string yer("*");
  std::string dow("*");

  is >> sec >> mnt >> hrs >> day >> mon >> yer >> dow;
  //TODO
  std::vector<std::string> retval;
  return retval;
}

void ScheduleRecord::parse(const std::string& rec)
{
  std::istringstream is(rec);

  std::string sec("*");
  std::string mnt("*");
  std::string hrs("*");
  std::string day("*");
  std::string mon("*");
  std::string yer("*");
  std::string dow("*");

  is >> sec >> mnt >> hrs >> day >> mon >> yer >> dow >> m_clientId;
  std::getline(is, m_task, '|'); // just to get rest of line with spaces
  
  m_tm.tm_sec = parseItem(sec, 0, 59);
  m_tm.tm_min = parseItem(mnt, 0, 59);
  m_tm.tm_hour = parseItem(hrs, 0, 23);
  m_tm.tm_mday = parseItem(day, 1, 31);
  m_tm.tm_mon = parseItem(mon, 1, 12) - 1;
  m_tm.tm_year = parseItem(yer, 0, 9000);
  m_tm.tm_wday = parseItem(dow, 0, 6);
}

int ScheduleRecord::parseItem(std::string& item, int mnm, int mxm)
{
  int num;
  try {
    num = std::stoi(item);
    if (num < mnm || num > mxm) {
      num = -1;
    }
  }
  catch (...) {
    num = -1; //TODO
  }
  return num;
}

system_clock::time_point ScheduleRecord::getNext(const std::chrono::system_clock::time_point& actualTimePoint, const std::tm& actualTime)
{
  system_clock::time_point tp;

  if (!m_periodic) {
    bool lower = false;
    std::tm c_tm = actualTime;

    while (true) {
      if (!compareTimeVal(c_tm.tm_sec, m_tm.tm_sec, lower))
        break;
      if (!compareTimeVal(c_tm.tm_min, m_tm.tm_min, lower))
        break;
      if (!compareTimeVal(c_tm.tm_hour, m_tm.tm_hour, lower))
        break;
      //Compare Day of Week if present
      if (m_tm.tm_wday >= 0) {
        int dif = c_tm.tm_wday - m_tm.tm_wday;
        if (dif == 0 && !lower)
          c_tm.tm_mday++;
        else if (dif > 0)
          c_tm.tm_mday += 7 - dif;
        else
          c_tm.tm_mday -= dif;
        break;
      }
      if (!compareTimeVal(c_tm.tm_mday, m_tm.tm_mday, lower))
        break;
      if (!compareTimeVal(c_tm.tm_mon, m_tm.tm_mon, lower))
        break;
      if (!compareTimeVal(c_tm.tm_year, m_tm.tm_year, lower))
        break;

      break;
    }

    std::time_t tt = std::mktime(&c_tm);
    if (tt == -1) {
      throw std::logic_error("Invalid time conversion");
    }
    tp = system_clock::from_time_t(tt);
  }
  else {
    if (m_started)
      tp = actualTimePoint + m_period;
    else {
      tp = m_startTime;
      m_started = true;
    }
  }
  
  //m_next = asString(tp);
  return tp;
}

//return true - continue false - finish
bool ScheduleRecord::compareTimeVal(int& cval, int tval, bool& lw) const
{
  int dif;
  if (tval >= 0) {
    lw = (dif = cval - tval) < 0 ? true : dif > 0 ? false : lw;
    cval = tval;
    return true;
  }
  else {
    if (!lw) cval++;
    return false;
  }
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

