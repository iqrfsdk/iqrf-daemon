#include "ClientServicePm.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

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

  m_daemon->getScheduler()->registerMessageHandler(m_name, [&](const std::string& msg) {
    ustring msgu((unsigned char*)msg.data(), msg.size());
    handleMsgFromMessaging(msgu);
  });

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
    "Received from MESSAGING: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //to encode output message
  std::ostringstream os;

  //get input message
  std::string msgs((const char*)msg.data(), msg.size());
  std::istringstream is(msgs);

  std::unique_ptr<DpaTask> dpaTask = m_serializer->parseRequest(msgs);
  if (dpaTask) {
    DpaTransactionTask trans(*dpaTask);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();
    os << dpaTask->encodeResponse(trans.getErrorStr());
  }
  else {
    os << m_serializer->getLastError();
  }

  ustring msgu((unsigned char*)os.str().data(), os.str().size());
  m_messaging->sendMessage(msgu);
}
