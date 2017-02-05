#include "ClientServicePm.h"
#include "PrfFrc.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const std::string SCHEDULED_SEND_FRC_TASK("SND_FRC");
const std::string SCHEDULED_SEND_PMT_TASK("SND_PMT");

ClientServicePm::ClientServicePm(const std::string & name)
  :m_name(name)
  , m_messaging(nullptr)
  , m_daemon(nullptr)
  , m_serializer(nullptr)
{
  m_taskQueue = std::unique_ptr<TaskQueue<PrfPulseMeterSchd*>>(
    ant_new TaskQueue<PrfPulseMeterSchd*>([this](PrfPulseMeterSchd* pm) {
    this->processPulseMeterFromTaskQueue(*pm);
  }));
}

ClientServicePm::~ClientServicePm()
{
}

void ClientServicePm::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
}

void ClientServicePm::setSerializer(ISerializer* serializer)
{
  m_serializer = serializer;
  m_messaging->registerMessageHandler([this](const ustring& msg) {
    this->handleMsgFromMessaging(msg);
  });
}

void ClientServicePm::setMessaging(IMessaging* messaging)
{
  m_messaging = messaging;
}

void ClientServicePm::start()
{
  TRC_ENTER("");

  //remove all possible configured tasks 
  m_daemon->getScheduler()->removeAllMyTasks(getClientName());

  //register task handler
  m_daemon->getScheduler()->registerMessageHandler(m_name, [this](const std::string& task) {
    this->handleTaskFromScheduler(task);
  });

  //TODO get from config
  PrfPulseMeterJson pmj1(1);
  PrfPulseMeterSchd pm1(pmj1, m_daemon->getScheduler());
  m_watchedPm.push_back(pm1);
  PrfPulseMeterSchd pm2(PrfPulseMeterJson(2), m_daemon->getScheduler());
  m_watchedPm.push_back(pm2);

  //schedule FRCs to get running nodes
  setFrc(true);

  TRC_INF("ClientServicePm :" << PAR(m_name) << " started");

  TRC_LEAVE("");
}

void ClientServicePm::stop()
{
  TRC_ENTER("");

  m_taskQueue.reset();

  TRC_INF("ClientServicePm :" << PAR(m_name) << " stopped");
  TRC_LEAVE("");
}

void ClientServicePm::handleTaskFromScheduler(const std::string& task)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from Scheduler: " << std::endl << task);

  if (SCHEDULED_SEND_FRC_TASK == task) {
    processFrcFromScheduler(task);
  }
  else if (task.find(SCHEDULED_SEND_PMT_TASK) != std::string::npos) {
    processPulseMeterFromScheduler(task);
  }
}

void ClientServicePm::handleMsgFromMessaging(const ustring& msg)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from Messaging: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //get input message
  std::string msgs((const char*)msg.data(), msg.size());
  //TODO
}

void ClientServicePm::setFrc(bool val)
{
  if (val == m_frcActive)
    return;

  if (val && !m_frcActive) {
    TRC_DBG("Start FRC");
    m_frcHandle = m_daemon->getScheduler()->scheduleTaskPeriodic(getClientName(), SCHEDULED_SEND_FRC_TASK, std::chrono::seconds(5));
    m_frcActive = true;
  }

  if (!val && m_frcActive) {
    TRC_DBG("Stop FRC");
    m_daemon->getScheduler()->removeTask(getClientName(), m_frcHandle);
    m_frcActive = false;
  }
}

//Process FRC sent from Scheduler
void ClientServicePm::processFrcFromScheduler(const std::string& task)
{
  //prepare FRC
  PrfFrc frc(PrfFrc::Cmd::SEND, PrfFrc::FrcCmd::Prebonding);
  DpaTransactionTask trans(frc);
  m_daemon->executeDpaTransaction(trans);
  int result = trans.waitFinish();

  TRC_DBG("Response: " << NAME_PAR(STATUS, trans.getErrorStr()));

  if (0 == trans.getError()) {
    bool stopFrc = true;
    for (auto& pm : m_watchedPm) {
      bool isSync = pm.isSync();
      if (!isSync && frc.getFrcData_bit2(pm.getDpa().getAddress()))
        m_taskQueue->pushToQueue(&pm);
      else if (!isSync)
        stopFrc = false; //not all synced
    }
    setFrc(!stopFrc);;
  }
}

//Process pulsemeter task from TaskQueue
//The tasks are stored there as results from FRC handling from Scheduler
void ClientServicePm::processPulseMeterFromTaskQueue(PrfPulseMeterSchd& pm)
{
  if (!pm.isSync()) {
    TRC_INF(std::endl << "PMETER: " << NAME_PAR(node, pm.getDpa().getAddress()) << " ALIVE !!!!!!!!!!!!!!!" << std::endl);
    pm.setSync(true);

    //Schedule next read and sleep
    std::ostringstream os;
    os << SCHEDULED_SEND_PMT_TASK << ' ' << pm.getDpa().getAddress();
    pm.scheduleTaskPeriodic(getClientName(), os.str(), std::chrono::seconds(10));
  }
}

void ClientServicePm::processPulseMeterFromScheduler(const std::string& task)
{
  std::string cmd;
  uint16_t adr = 0;
  std::istringstream is(task);
  is >> cmd >> adr;

  if (adr == 0) {
    TRC_WAR("Invalid address: " << PAR(adr));
    return;
  }

  PrfPulseMeterSchd* pms(nullptr);
  for (auto& pm : m_watchedPm) {
    if (pm.getDpa().getAddress() == adr) {
      pms = &pm;
      break;
    }
  }

  if (nullptr == pms) {
    TRC_WAR("Unknown address: " << PAR(adr));
    return;
  }
  
  //send read
  DpaTransactionTask trans(pms->getDpa());
  m_daemon->executeDpaTransaction(trans);
  int result = trans.waitFinish();
  
  //to encode output message
  std::ostringstream os;
  os << pms->getDpa().encodeResponse(trans.getErrorStr());

  ustring msgu((unsigned char*)os.str().data(), os.str().size());
  m_messaging->sendMessage(msgu);

  if (result != 0) {
    pms->setSync(false);
    setFrc(true);
  }
}
