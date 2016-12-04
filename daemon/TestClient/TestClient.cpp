#include "TestClient.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

TestClient::TestClient()
  :m_daemon(nullptr)
{
}

TestClient::~TestClient()
{
}

void TestClient::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
}

void TestClient::start()
{
  TRC_ENTER("");

  //TODO load Messaging plugin
  m_messaging = ant_new MqttMessaging();
  m_messaging->registerMessageHandler([&](const ustring& msg) {
    handleMsgFromMessaging(msg);
  });

  //TODO load Serializer plugin
  m_serializer = ant_new DpaTaskSimpleSerializerFactory();

  std::cout << "TestClient started" << std::endl;

  TRC_LEAVE("");
}

void TestClient::stop()
{
  TRC_ENTER("");

  std::cout << "TestClient stopped" << std::endl;
  TRC_LEAVE("");
}

void TestClient::handleMsgFromMessaging(const ustring& msg)
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
