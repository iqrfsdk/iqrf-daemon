#include "IqrfappMqMessaging.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "DpaTransactionTask.h"
#include "MqChannel.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

const std::string MQ_ERROR_ADDRESS("ERROR_ADDRESS");
const std::string MQ_ERROR_DEVICE("ERROR_DEVICE");

IqrfappMqMessaging::IqrfappMqMessaging()
  :m_daemon(nullptr)
  , m_mqChannel(nullptr)
  , m_toMqMessageQueue(nullptr)
{
  m_localMqName = "iqrf-daemon-110";
  m_remoteMqName = "iqrf-daemon-100";
}

IqrfappMqMessaging::~IqrfappMqMessaging()
{
}

void IqrfappMqMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

void IqrfappMqMessaging::start()
{
  TRC_ENTER("");

  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  std::cout << "daemon-MQ-protocol started" << std::endl;

  TRC_LEAVE("");
}

void IqrfappMqMessaging::stop()
{
  TRC_ENTER("");
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  std::cout << "daemon-MQ-protocol stopped" << std::endl;
  TRC_LEAVE("");
}

void IqrfappMqMessaging::sendMessageToMq(const std::string& message)
{
  ustring msg((unsigned char*)message.data(), message.size());
  TRC_DBG(FORM_HEX(message.data(), message.size()));
  m_toMqMessageQueue->pushToQueue(msg);
}

int IqrfappMqMessaging::handleMessageFromMq(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQ: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  //to encode output message
  std::ostringstream os;

  //get input message
  std::string msg((const char*)mqMessage.data(), mqMessage.size());
  std::istringstream is(msg);

  std::unique_ptr<DpaTask> dpaTask = m_factory.parse(msg);
  if (dpaTask) {
    DpaTransactionTask trans(*dpaTask);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();
    dpaTask->encodeResponseMessage(os, trans.getErrorStr());
  }
  else {
    os << MQ_ERROR_DEVICE;
  }

  sendMessageToMq(os.str());

  return 1;
}
