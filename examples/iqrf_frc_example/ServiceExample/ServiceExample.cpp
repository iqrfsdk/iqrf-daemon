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

#include "JsonSerializer.h"
#include "LaunchUtils.h"
#include "ServiceExample.h"
#include "PrfFrc.h"
#include "PrfOs.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

INIT_COMPONENT(IService, ServiceExample)

const std::string SCHEDULED_SEND_FRC_TASK("SND_FRC");
const std::string SCHEDULED_SEND_THM_TASK("SND_THM");

ServiceExample::ServiceExample(const std::string & name)
  :m_name(name)
  , m_messaging(nullptr)
  , m_daemon(nullptr)
  , m_serializer(nullptr)
{
  m_taskQueue = std::unique_ptr<TaskQueue<PrfThermometerSchd*>>(
    ant_new TaskQueue<PrfThermometerSchd*>([this](PrfThermometerSchd* pm) {
    this->processPrfThermometerFromTaskQueue(*pm);
  }));
}

ServiceExample::~ServiceExample()
{
}

void ServiceExample::update(const rapidjson::Value& cfg)
{
  updateConfiguration(cfg);
}

//------------------------
void ServiceExample::updateConfiguration(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  jutils::assertIsObject("", cfg);

  m_frcPeriod = jutils::getPossibleMemberAs<int>("FrcPeriod", cfg, m_frcPeriod);
  m_sleepPeriod = jutils::getPossibleMemberAs<int>("SleepPeriod", cfg, m_sleepPeriod);

  const auto v = jutils::getMember("DpaRequestJsonPattern", cfg);
  if (!v->value.IsObject())
    THROW_EX(std::logic_error, "Expected: Json Object, detected: " << NAME_PAR(name, v->value.GetString()) << NAME_PAR(type, v->value.GetType()));

  std::vector<int> vect = jutils::getMemberAsVector<int>("Thermometers", cfg);
  for (int node : vect) {
    PrfThermometerJson prfThermometerJson(v->value);
    prfThermometerJson.setAddress(node);
    PrfThermometerSchd pm(prfThermometerJson, m_daemon->getScheduler());
    m_watchedThermometers.push_back(pm);
  }

  TRC_LEAVE("");
}

void ServiceExample::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
}

void ServiceExample::setSerializer(ISerializer* serializer)
{
  m_serializer = serializer;
  m_messaging->registerMessageHandler([this](const ustring& msg) {
    this->handleMsgFromMessaging(msg);
  });
}

void ServiceExample::setMessaging(IMessaging* messaging)
{
  m_messaging = messaging;
}

void ServiceExample::start()
{
  TRC_ENTER("");

  //remove all possible configured tasks
  m_daemon->getScheduler()->removeAllMyTasks(getName());

  //register task handler
  m_daemon->getScheduler()->registerMessageHandler(m_name, [this](const std::string& task) {
    this->handleTaskFromScheduler(task);
  });

  //schedule FRCs to get running nodes
  setFrc(true);

  TRC_INF("ServiceExample :" << PAR(m_name) << " started");

  TRC_LEAVE("");
}

void ServiceExample::stop()
{
  TRC_ENTER("");

  m_taskQueue.reset();

  TRC_INF("ServiceExample :" << PAR(m_name) << " stopped");
  TRC_LEAVE("");
}

void ServiceExample::handleTaskFromScheduler(const std::string& task)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from Scheduler: " << std::endl << task);

  if (SCHEDULED_SEND_FRC_TASK == task) {
    processFrcFromScheduler(task);
  }
  else if (task.find(SCHEDULED_SEND_THM_TASK) != std::string::npos) {
    processPrfThermometerFromScheduler(task);
  }
}

void ServiceExample::handleMsgFromMessaging(const ustring& msg)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from Messaging: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //get input message
  std::string msgs((const char*)msg.data(), msg.size());
  //TODO
}

void ServiceExample::setFrc(bool val)
{
  if (val == m_frcActive)
    return;

  if (val && !m_frcActive) {
    TRC_DBG("Start FRC");
    m_frcHandle = m_daemon->getScheduler()->scheduleTaskPeriodic(getName(),
      SCHEDULED_SEND_FRC_TASK, std::chrono::seconds(m_frcPeriod));
    m_frcActive = true;
  }

  if (!val && m_frcActive) {
    TRC_DBG("Stop FRC");
    m_daemon->getScheduler()->removeTask(getName(), m_frcHandle);
    m_frcActive = false;
  }
}

//Process FRC sent from Scheduler
void ServiceExample::processFrcFromScheduler(const std::string& task)
{
  //prepare FRC
  PrfFrc frc(PrfFrc::Cmd::SEND, PrfFrc::FrcCmd::Prebonding);
  DpaTransactionTask trans(frc);
  m_daemon->executeDpaTransaction(trans);
  int result = trans.waitFinish();

  TRC_DBG("Response: " << NAME_PAR(STATUS, trans.getErrorStr()));

  if (0 == trans.getError()) {
    bool stopFrc = true;
    for (auto& pm : m_watchedThermometers) {
      bool isSync = pm.isSync();
      if (!isSync && frc.getFrcData_bit2(pm.getDpa().getAddress()))
        m_taskQueue->pushToQueue(&pm);
      else if (!isSync)
        stopFrc = false; //not all synced
    }
    setFrc(!stopFrc);;
  }
}

//Process thermometer task from TaskQueue
//The tasks are stored there as results from FRC handling from Scheduler
void ServiceExample::processPrfThermometerFromTaskQueue(PrfThermometerSchd& pm)
{
  if (!pm.isSync()) {
    TRC_INF(std::endl << "PMETER: " << NAME_PAR(node, pm.getDpa().getAddress()) << " ALIVE !!!!!!!!!!!!!!!" << std::endl);
    pm.setSync(true);

    //Schedule next read
    std::ostringstream os;
    os << SCHEDULED_SEND_THM_TASK << ' ' << pm.getDpa().getAddress();
    pm.scheduleTaskPeriodic(getName(), os.str(), std::chrono::seconds(m_sleepPeriod));
  }
}

void ServiceExample::processPrfThermometerFromScheduler(const std::string& task)
{
  //parse cmd and address from scheduled task
  std::string cmd;
  uint16_t adr = 0;
  std::istringstream is(task);
  is >> cmd >> adr;

  if (adr == 0) {
    TRC_WAR("Invalid address: " << PAR(adr));
    return;
  }

  //find according address from watched thermometers
  PrfThermometerSchd* prfThermometerSchd(nullptr);
  for (auto& tm : m_watchedThermometers) {
    if (tm.getDpa().getAddress() == adr) {
      prfThermometerSchd = &tm;
      break;
    }
  }

  if (nullptr == prfThermometerSchd) {
    TRC_WAR("Unknown address: " << PAR(adr));
    return;
  }

  //send read
  int sleepPeriod = m_sleepPeriod > 2 ? m_sleepPeriod - 2 : 0;
  prfThermometerSchd->getDpa().setCmd(PrfThermometer::Cmd::READ);
  DpaTransactionTask transRead(prfThermometerSchd->getDpa());
  m_daemon->executeDpaTransaction(transRead);
  int resultRead = transRead.waitFinish();

  //send sleep
  PrfOs prfOs(prfThermometerSchd->getDpa().getAddress());
  prfOs.sleep(std::chrono::milliseconds(sleepPeriod * 1000), (uint8_t)PrfOs::TimeControl::LEDG_FLASH);
  DpaTransactionTask transSleep(prfOs);
  m_daemon->executeDpaTransaction(transSleep);
  int resultSleep = transSleep.waitFinish();

  TRC_DBG(">>>>>>>>>>>>>>>>>> Thermometer result: " << NAME_PAR(TransactionError, transRead.getErrorStr())
    << NAME_PAR(TransactionError, prfThermometerSchd->getDpa().getAddress()));

  switch (resultRead) {
    case 0:
    {
      //encode output message
      std::ostringstream os;
      os << prfThermometerSchd->getDpa().encodeResponse(transRead.getErrorStr());

      ustring msgu((unsigned char*)os.str().data(), os.str().size());
      m_messaging->sendMessage(msgu);
    }
    break;

    case -1: //ERROR_TIMEOUT
    {
      //we probably lost pmeter - start FRC to sync again
      prfThermometerSchd->removeSchedule(getName());
      TRC_DBG("Lost Thermometer");
      prfThermometerSchd->setSync(false);
      setFrc(true);
    }
    break;

    case ERROR_PNUM:
    {
      //there isn't pmeter device in the address - stop trying
      prfThermometerSchd->removeSchedule(getName());
      TRC_DBG("Stop seeking Thermometer");
    }
    break;

    default:
    { //other error
      prfThermometerSchd->removeSchedule(getName());
      TRC_DBG("Stop seeking Thermometer");
    }
  }
}
