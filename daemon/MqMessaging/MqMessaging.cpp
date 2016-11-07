#include "DpaTask.h"
#include "DpaMessage.h"
#include "MqMessaging.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <climits>
#include <ctime>
#include <ratio>
#include <chrono>

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

void MqMessaging::sendDpaMessageToMq(const DpaMessage&  dpaMessage)
{
  ustring message(dpaMessage.DpaPacketData(), dpaMessage.Length());
  TRC_DBG(FORM_HEX(message.data(), message.size()));
  
  m_toMqMessageQueue->pushToQueue(message);
}

void MqMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

int MqMessaging::handleMessageFromMq(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQ: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  m_transaction->setMessage(mqMessage);
  m_daemon->executeDpaTransaction(*m_transaction);

  return 0;
}

MqMessaging::MqMessaging()
  :m_daemon(nullptr)
  , m_mqChannel(nullptr)
  , m_toMqMessageQueue(nullptr)
{
  m_localMqName = "iqrf-daemon-110";
  m_remoteMqName = "iqrf-daemon-100";
}

MqMessaging::~MqMessaging()
{
}

void MqMessaging::start()
{
  TRC_ENTER("");

  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  m_transaction = ant_new MqMessagingTransaction(this);

  std::cout << "daemon-MQ-protocol started" << std::endl;

  TRC_LEAVE("");
}

void MqMessaging::stop()
{
  TRC_ENTER("");
  std::cout << "udp-daemon-protocol stops" << std::endl;
  delete m_transaction;
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  TRC_LEAVE("");
}

////////////////////////////////////
MqMessagingTransaction::MqMessagingTransaction(MqMessaging* mqMessaging)
  :m_mqMessaging(mqMessaging)
{
}

MqMessagingTransaction::~MqMessagingTransaction()
{
}

const DpaMessage& MqMessagingTransaction::getMessage() const
{
  return m_message;
}

void MqMessagingTransaction::processConfirmationMessage(const DpaMessage& confirmation)
{
  m_mqMessaging->sendDpaMessageToMq(confirmation);
}

void MqMessagingTransaction::processResponseMessage(const DpaMessage& response)
{
  m_mqMessaging->sendDpaMessageToMq(response);
}

void MqMessagingTransaction::processFinish(DpaRequest::DpaRequestStatus status)
{
  m_success = this->isProcessed(status);
}

void MqMessagingTransaction::setMessage(ustring message)
{
  m_message.DataToBuffer(message.data(), message.length());
}
