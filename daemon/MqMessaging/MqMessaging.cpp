#include "MqMessaging.h"
#include "MqChannel.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

MqMessaging::MqMessaging()
  :m_daemon(nullptr)
  , m_mqChannel(nullptr)
  , m_toMqMessageQueue(nullptr)
  , m_name("MqMessaging")
{
  m_localMqName = "iqrf-daemon-110";
  m_remoteMqName = "iqrf-daemon-100";
}

MqMessaging::~MqMessaging()
{
}

void MqMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

void MqMessaging::start()
{
  TRC_ENTER("");

  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  TRC_INF("daemon-MQ-protocol started");

  TRC_LEAVE("");
}

void MqMessaging::stop()
{
  TRC_ENTER("");
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  TRC_INF("daemon-MQ-protocol stopped");
  TRC_LEAVE("");
}

void MqMessaging::registerMessageHandler(MessageHandlerFunc hndl)
{
  m_messageHandlerFunc = hndl;
}

void MqMessaging::unregisterMessageHandler()
{
  m_messageHandlerFunc = IMessaging::MessageHandlerFunc();
}

void MqMessaging::sendMessage(const ustring& msg)
{
  TRC_DBG(FORM_HEX(msg.data(), msg.size()));
  m_toMqMessageQueue->pushToQueue(msg);
}

int MqMessaging::handleMessageFromMq(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQ: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  if (m_messageHandlerFunc)
    m_messageHandlerFunc(mqMessage);

  return 0;
}
