#include "IqrfappMqMessaging.h"
#include "DpaTransactionTask.h"
//#include "DpaTask.h"
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

  //get input message
  std::string msg((const char*)mqMessage.data(), mqMessage.size());
  std::istringstream is(msg);

  //parse input message
  std::string device;
  int address(-1);
  is >> device >> address;

  //check delivered address
  if (address < 0) {
    sendMessageToMq(MQ_ERROR_ADDRESS);
    return -1;
  }

  //to encode output message
  std::ostringstream os;

  if (device == NAME_Thermometer) {
    DpaThermometer temp(address);
    DpaTransactionTask trans(temp);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << temp;
    else
      os << trans.getErrorStr();
  }
  else if (device == NAME_PulseLedG) {
    DpaPulseLedG pulse(address);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else if (device == NAME_PulseLedR) {
    DpaPulseLedR pulse(address);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else {
    os << MQ_ERROR_DEVICE;
  }

  sendMessageToMq(os.str());

  return 0;
}
