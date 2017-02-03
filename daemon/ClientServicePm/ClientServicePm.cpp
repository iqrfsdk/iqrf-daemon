#include "ClientServicePm.h"
#include "PrfFrc.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const std::string SEND_FRC_TASK("SEND_FRC_TASK");

ClientServicePm::ClientServicePm(const std::string & name)
  :m_name(name)
  , m_messaging(nullptr)
  , m_daemon(nullptr)
  , m_serializer(nullptr)
{
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
  m_messaging->registerMessageHandler([&](const ustring& msg) {
    handleMsgFromMessaging(msg);
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
  m_daemon->getScheduler()->registerMessageHandler(m_name, [&](const std::string& msg) {
    ustring msgu((unsigned char*)msg.data(), msg.size());
    handleMsgFromMessaging(msgu);
  });

  //schedule FRCs to get running nodes
  m_daemon->getScheduler()->scheduleTaskPeriodic(getClientName(), SEND_FRC_TASK, std::chrono::seconds(5));

  //TODO get from config
  m_watchedPm.insert(std::make_pair(1,false));
  m_watchedPm.insert(std::make_pair(2, false));

  TRC_INF("ClientServicePm :" << PAR(m_name) << " started");

  TRC_LEAVE("");
}

void ClientServicePm::stop()
{
  TRC_ENTER("");

  TRC_INF("ClientServicePm :" << PAR(m_name) << " stopped");
  TRC_LEAVE("");
}

void ClientServicePm::handleMsgFromMessaging(const ustring& msg)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from Scheduler: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //get input message
  std::string msgs((const char*)msg.data(), msg.size());
  
  if (SEND_FRC_TASK == msgs) {
    //prepare FRC
    PrfFrc frc(PrfFrc::Cmd::SEND, PrfFrc::FrcCmd::Prebonding);
    DpaTransactionTask trans(frc);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    TRC_DBG("Response: " << NAME_PAR(STATUS, trans.getErrorStr()));
    
    if (0 == trans.getError()) {
      for (auto& it : m_watchedPm) {
        it.second = frc.getFrcData_bit2(it.first);
        if (it.second) {
          TRC_INF(std:: endl << "PMETER: " << NAME_PAR(node, it.first) << " ALIVE !!!!!!!!!!!!!!!" << std::endl);
        }
      }
    }
  }
}
