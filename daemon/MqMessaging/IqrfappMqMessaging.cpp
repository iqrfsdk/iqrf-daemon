#include "DpaTask.h"
#include "DpaMessage.h"
#include "IqrfappMqMessaging.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <climits>
#include <ctime>
#include <ratio>
#include <chrono>

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

void IqrfappMqMessaging::sendMessageToMq(const std::string& message)
{
  ustring msg((unsigned char*)message.data(), message.size());
  TRC_DBG(FORM_HEX(message.data(), message.size()));
  m_toMqMessageQueue->pushToQueue(msg);
}

void IqrfappMqMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

int IqrfappMqMessaging::handleMessageFromMq(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQ: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  std::string msg((const char*)mqMessage.data(), mqMessage.size());
  std::istringstream is(msg);

  const std::string MQ_ERROR_ADDRESS("error: address");
  const std::string MQ_ERROR_DEVICE("error: device");
  const std::string MQ_ERROR_TIMEOUT("error: timeout");

  std::string device;
  int address(-1);

  is >> device >> address;

  if (address < 0) {
    sendMessageToMq(MQ_ERROR_ADDRESS);
    return -1;
  }

  if (device == "temp") {
    DpaThermometer temp(address);
    m_task = &temp;
    m_promise = std::promise<bool>();
    m_daemon->executeDpaTransaction(*this);
    m_success = isReady();

    std::ostringstream os;
    if (m_success) {
      os << "temp " << temp.getTemperature();
      sendMessageToMq(os.str());
    }
    else
      sendMessageToMq(MQ_ERROR_TIMEOUT);
  }
  else if (device == "pulseG") {
    DpaPulseLed pulse(address, DpaPulseLed::kLedGreen);
    m_task = &pulse;
    m_promise = std::promise<bool>();
    m_daemon->executeDpaTransaction(*this);
    m_success = isReady();

    std::ostringstream os;
    if (m_success) {
      os << "pulseG: OK";
      sendMessageToMq(os.str());
    }
    else
      sendMessageToMq(MQ_ERROR_TIMEOUT);
  }
  else if (device == "pulseR") {
    DpaPulseLed pulse(address, DpaPulseLed::kLedGreen);
    m_task = &pulse;
    m_promise = std::promise<bool>();
    m_daemon->executeDpaTransaction(*this);
    m_success = isReady();

    std::ostringstream os;
    if (m_success) {
      os << "pulseG: OK";
      sendMessageToMq(os.str());
    }
    else
      sendMessageToMq(MQ_ERROR_TIMEOUT);
  }
  else {
    sendMessageToMq(MQ_ERROR_DEVICE);
    return -1;
  }
  return 0;
}

IqrfappMqMessaging::IqrfappMqMessaging()
  :m_daemon(nullptr)
  , m_task(nullptr)
  , m_success(false)
  , m_mqChannel(nullptr)
  , m_toMqMessageQueue(nullptr)
{
  m_localMqName = "iqrf-daemon-110";
  m_remoteMqName = "iqrf-daemon-100";
}

IqrfappMqMessaging::~IqrfappMqMessaging()
{
}

void IqrfappMqMessaging::start()
{
  TRC_ENTER("");

  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE);

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
  std::cout << "udp-daemon-protocol stops" << std::endl;
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  TRC_LEAVE("");
}

////////////////////////////////////
const DpaMessage& IqrfappMqMessaging::getMessage() const
{
  return m_task->getRequest();
}

void IqrfappMqMessaging::processConfirmationMessage(const DpaMessage& confirmation)
{
}

void IqrfappMqMessaging::processResponseMessage(const DpaMessage& response)
{
  m_task->parseResponse(response);
}

bool IqrfappMqMessaging::isReady()
{
  return m_promise.get_future().get();
}

void IqrfappMqMessaging::processFinish(DpaRequest::DpaRequestStatus status)
{
  m_success = isProcessed(status);
  std::promise<bool> pr2(std::move(m_promise));
  pr2.set_value(m_success);
  //m_promise.set_value(m_success);
}
