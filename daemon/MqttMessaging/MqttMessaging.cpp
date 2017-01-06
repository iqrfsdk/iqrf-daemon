#include "MqttMessaging.h"
#include "TaskQueue.h"
#include "MQTTClient.h"
#include "PlatformDep.h"
#include "IDaemon.h"
#include "IqrfLogging.h"
#include <string.h>
#include <atomic>

class Impl {
public:
  Impl()
    :m_daemon(nullptr)
    ,m_toMqttMessageQueue(nullptr)
  {}

  ~Impl()
  {}

  void start();
  void stop();
  void registerMessageHandler(IMessaging::MessageHandlerFunc hndl) {
    m_messageHandlerFunc = hndl;
  }

  const std::string& getName() const { return m_mqttClientId; }

  void updateConfiguration(const rapidjson::Value& cfg);

  void unregisterMessageHandler() {
    m_messageHandlerFunc = IMessaging::MessageHandlerFunc();
  }

  void sendMessage(const ustring& msg) {
    m_toMqttMessageQueue->pushToQueue(msg);
  }

  static void sdelivered(void *context, MQTTClient_deliveryToken dt) {
    ((Impl*)context)->delivered(dt);
  }

  static int smsgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    return ((Impl*)context)->msgarrvd(topicName, topicLen, message);
  }

  static void sconnlost(void *context, char *cause) {
    ((Impl*)context)->connlost(cause);
  }

  void handleMessageFromMqtt(const ustring& mqMessage);

  void delivered(MQTTClient_deliveryToken dt)
  {
    TRC_DBG("Message delivery confirmed" << PAR(dt));
    m_deliveredtoken = dt;
  }

  int msgarrvd(char *topicName, int topicLen, MQTTClient_message *message)
  {
    ustring msg((unsigned char*)message->payload, message->payloadlen);
    if (!strncmp(topicName, m_mqttTopicRequest.c_str(), topicLen))
      handleMessageFromMqtt(msg);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
  }

  void connlost(char *cause)
  {
    TRC_WAR("Connection lost: " << NAME_PAR(cause, (cause ? cause : "nullptr")));
    m_connected = false;
  }

  void sendTo(const ustring& msg);

  IDaemon* m_daemon;
  TaskQueue<ustring>* m_toMqttMessageQueue;
  std::atomic_bool m_connected;
  std::atomic<MQTTClient_deliveryToken> m_deliveredtoken;
  MQTTClient m_client;
  
  IMessaging::MessageHandlerFunc m_messageHandlerFunc;

  //configuration
  std::string m_mqttBrokerAddr;
  std::string m_mqttClientId;
  bool m_enabled = false;
  int m_mqttPersistence = 0;
  std::string m_mqttTopicRequest;
  std::string m_mqttTopicResponse;
  int m_mqttQos = 0;
  unsigned long m_mqttTimeout = 10000;
  std::string m_mqttUser;
  std::string m_mqttPassword;
  bool m_mqttEnabledSSL = false;
};

MqttMessaging::MqttMessaging()
{
  m_impl = ant_new Impl();
}

MqttMessaging::~MqttMessaging()
{
  delete m_impl;
}

void MqttMessaging::updateConfiguration(const rapidjson::Value& cfg)
{
  m_impl->updateConfiguration(cfg);
}

void MqttMessaging::setDaemon(IDaemon* daemon)
{
  m_impl->m_daemon = daemon;
  m_impl->m_daemon->registerMessaging(*this);
}

void MqttMessaging::start()
{
  m_impl->start();
}

void MqttMessaging::stop()
{
  m_impl->stop();
}

void MqttMessaging::registerMessageHandler(MessageHandlerFunc hndl)
{
  m_impl->registerMessageHandler(hndl);
}

void MqttMessaging::unregisterMessageHandler()
{
  m_impl->unregisterMessageHandler();
}

void MqttMessaging::sendMessage(const ustring& msg)
{
  m_impl->sendMessage(msg);
}

const std::string& MqttMessaging::getName() const
{
  return m_impl->getName();
}

void Impl::updateConfiguration(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  jutils::assertIsObject("", cfg);

  m_mqttBrokerAddr = jutils::getMemberAs<std::string>("BrokerAddr", cfg);
  m_mqttClientId = jutils::getMemberAs<std::string>("ClientId", cfg);
  m_enabled = jutils::getMemberAs<bool>("Enabled", cfg);
  m_mqttPersistence = jutils::getMemberAs<int>("Persistence", cfg);
  m_mqttQos = jutils::getMemberAs<int>("Qos", cfg);
  m_mqttTimeout = (unsigned long)jutils::getMemberAs<int>("Timeout:", cfg);
  m_mqttTopicRequest = jutils::getMemberAs<std::string>("TopicRequest", cfg);
  m_mqttTopicResponse = jutils::getMemberAs<std::string>("TopicResponse", cfg);
  m_mqttUser = jutils::getMemberAs<std::string>("User", cfg);
  m_mqttPassword = jutils::getMemberAs<std::string>("Password", cfg);
  m_mqttEnabledSSL= jutils::getMemberAs<bool>("EnabledSSL", cfg);

  TRC_LEAVE("");
}

void Impl::start()
{
  TRC_ENTER("");

  if (!m_enabled) {
    TRC_WAR("MqttMessaging DISABLED:" << PAR(m_mqttClientId));
    TRC_LEAVE("");
    return;
  }

  m_toMqttMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    sendTo(msg);
   });

  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
  ssl_opts.enableServerCertAuth = true;
  int retval;

  if ((retval = MQTTClient_create(&m_client, m_mqttBrokerAddr.c_str(),
    m_mqttClientId.c_str(), m_mqttPersistence, NULL)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_create() failed: " << PAR(retval));
  }

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.connectTimeout = 5;
  conn_opts.username = m_mqttUser.c_str();
  conn_opts.password = m_mqttPassword.c_str();
  
  if (m_mqttEnabledSSL) {
    conn_opts.ssl = &ssl_opts;
  }

  if ((retval = MQTTClient_setCallbacks(m_client, this, sconnlost, smsgarrvd, sdelivered)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_setCallbacks() failed: " << PAR(retval));
  }

  TRC_DBG("Connecting: " << PAR(m_mqttBrokerAddr) << PAR(m_mqttClientId));
  if ((retval = MQTTClient_connect(m_client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_connect() failed: " << PAR(retval));
  }

  TRC_DBG("Subscribing: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
  if ((retval = MQTTClient_subscribe(m_client, m_mqttTopicRequest.c_str(), m_mqttQos)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_subscribe() failed: " << PAR(retval));
  }

  m_connected = true;

  std::cout << "daemon-MQTT-protocol started" << std::endl;

  TRC_LEAVE("");
}

void Impl::stop()
{
  TRC_ENTER("");
  if (m_enabled) {
    MQTTClient_disconnect(m_client, 10000);
    MQTTClient_destroy(&m_client);
    delete m_toMqttMessageQueue;
    std::cout << "daemon-MQTT-protocol stopped" << std::endl;
  }
  TRC_LEAVE("");
}

void Impl::handleMessageFromMqtt(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQTT: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  if (m_messageHandlerFunc)
    m_messageHandlerFunc(mqMessage);
}

void Impl::sendTo(const ustring& msg)
{
  TRC_DBG("Sending to MQTT: " << NAME_PAR(topic, m_mqttTopicResponse) << std::endl <<
    FORM_HEX(msg.data(), msg.size()));

  MQTTClient_deliveryToken token;
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  int retval;

  pubmsg.payload = (void*)msg.data();
  pubmsg.payloadlen = (int)msg.size();
  pubmsg.qos = m_mqttQos;
  pubmsg.retained = 0;
  
  MQTTClient_publishMessage(m_client, m_mqttTopicResponse.c_str(), &pubmsg, &token);
  TRC_DBG("Waiting for publication: " << PAR(m_mqttTimeout));
  retval = MQTTClient_waitForCompletion(m_client, token, m_mqttTimeout);
}
