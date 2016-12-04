#include "TestClient.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "DpaTransactionTask.h"
#include "MqChannel.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

const std::string MQ_ERROR_ADDRESS("ERROR_ADDRESS");
const std::string MQ_ERROR_DEVICE("ERROR_DEVICE");

using namespace std::chrono;

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
  m_messaging->registerMessageHandler();

  //TODO load Serializer plugin
  m_serializer = ant_new DpaTaskSimpleSerializerFactory();

  /*
  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });
  */

  //m_dpaTaskQueue = ant_new TaskQueue<ScheduleRecord>([&](const ScheduleRecord& record) {
  //  handleScheduledRecord(record);
  //});

  std::cout << "TestClient started" << std::endl;

  TRC_LEAVE("");
}

void TestClient::stop()
{
  TRC_ENTER("");

  std::cout << "Scheduler stopped" << std::endl;
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

//void SchedulerMessaging::sendMessageToMq(const std::string& message)
//{
//  ustring msg((unsigned char*)message.data(), message.size());
//  TRC_DBG(FORM_HEX(message.data(), message.size()));
//  m_toMqMessageQueue->pushToQueue(msg);
//}
