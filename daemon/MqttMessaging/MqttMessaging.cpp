#include "MqttMessaging.h"
#include "TaskQueue.h"
#include "MQTTClient.h"
#include "PlatformDep.h"
#include "IDaemon.h"
#include "IqrfLogging.h"
#include <string.h>
#include <atomic>

const std::string MQTT_BROKER_ADDRESS("tcp://192.168.1.26:1883");
//const std::string MQTT_BROKER_ADDRESS("tcp://192.168.1.27:1883");
const std::string MQTT_CLIENTID("IqrfDpaMessaging");
const std::string MQTT_TOPIC_DPA_REQUEST("Iqrf/DpaRequest");
const std::string MQTT_TOPIC_DPA_RESPONSE("Iqrf/DpaResponse");
const int MQTT_QOS(1);
const unsigned long MQTT_TIMEOUT(10000);

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
    if (!strncmp(topicName, MQTT_TOPIC_DPA_REQUEST.c_str(), topicLen))
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
};

MqttMessaging::MqttMessaging()
{
  m_impl = ant_new Impl();
}

MqttMessaging::~MqttMessaging()
{
  delete m_impl;
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

////////////////////// Impl //////////////////
void Impl::start()
{
  TRC_ENTER("");

  m_toMqttMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    sendTo(msg);
   });

  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  int retval;

  MQTTClient_create(&m_client, MQTT_BROKER_ADDRESS.c_str(), MQTT_CLIENTID.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.connectTimeout = 5;

  MQTTClient_setCallbacks(m_client, this, sconnlost, smsgarrvd, sdelivered);

  if ((retval = MQTTClient_connect(m_client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_connect() failed: " << PAR(retval));
  }

  TRC_DBG("Subscribing: " << PAR(MQTT_TOPIC_DPA_REQUEST) << PAR(MQTT_CLIENTID) << PAR(MQTT_QOS));
  if ((retval = MQTTClient_subscribe(m_client, MQTT_TOPIC_DPA_REQUEST.c_str(), MQTT_QOS)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_subscribe() failed: " << PAR(retval));
  }

  m_connected = true;

  std::cout << "daemon-MQTT-protocol started" << std::endl;

  TRC_LEAVE("");
}

void Impl::stop()
{
  TRC_ENTER("");
  MQTTClient_disconnect(m_client, 10000);
  MQTTClient_destroy(&m_client);
  delete m_toMqttMessageQueue;
  std::cout << "daemon-MQTT-protocol stopped" << std::endl;
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
  MQTTClient_deliveryToken token;
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  int retval;

  pubmsg.payload = (void*)msg.data();
  pubmsg.payloadlen = (int)msg.size();
  pubmsg.qos = MQTT_QOS;
  pubmsg.retained = 0;
  
  MQTTClient_publishMessage(m_client, MQTT_TOPIC_DPA_RESPONSE.c_str(), &pubmsg, &token);
  TRC_DBG("Waiting for publication: " << PAR(MQTT_TIMEOUT));
  retval = MQTTClient_waitForCompletion(m_client, token, MQTT_TIMEOUT);
}
