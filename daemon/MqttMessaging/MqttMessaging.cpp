#include "MqttMessaging.h"
#include "SimpleSerializer.h"
#include "TaskQueue.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "DpaTransactionTask.h"
#include "MQTTClient.h"
#include "PlatformDep.h"
#include "IScheduler.h"
#include "IDaemon.h"
#include "IqrfLogging.h"
#include <string.h>
#include <atomic>

const std::string MQ_ERROR_ADDRESS("ERROR_ADDRESS");
const std::string MQ_ERROR_DEVICE("ERROR_DEVICE");

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
    ,m_scheduler(nullptr)
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

  void Impl::sendMessage(const ustring& msg) {
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

  //void sendMessageToMqtt(const std::string& message);
  int handleMessageFromMqtt(const ustring& mqMessage);
  //int handleMessageFromMqtt1(const ustring& mqMessage);

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

  void setScheduler(IScheduler* scheduler);
  void resetScheduler();
  void handleSchedulerResponse(const std::string& response);

  IDaemon* m_daemon;
  TaskQueue<ustring>* m_toMqttMessageQueue;
  std::atomic_bool m_connected;
  std::atomic<MQTTClient_deliveryToken> m_deliveredtoken;
  MQTTClient m_client;
  
  IScheduler* m_scheduler;
  DpaTaskSimpleSerializerFactory m_factory;

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
  m_impl->setScheduler(m_impl->m_daemon->getScheduler());
  m_impl->start();
}

void MqttMessaging::stop()
{
  m_impl->resetScheduler();
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

  //m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqttMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    sendTo(msg);
    //m_mqChannel->sendTo(msg);
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
  //delete m_mqChannel;
  MQTTClient_disconnect(m_client, 10000);
  MQTTClient_destroy(&m_client);
  delete m_toMqttMessageQueue;
  std::cout << "daemon-MQTT-protocol stopped" << std::endl;
  TRC_LEAVE("");
}

//void Impl::sendMessage(const ustring& msg)
//{
//  //ustring msg((unsigned char*)message.data(), message.size());
//  //TRC_DBG(FORM_HEX(message.data(), message.size()));
//  m_toMqttMessageQueue->pushToQueue(msg);
//}

int Impl::handleMessageFromMqtt(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQTT: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  if (m_messageHandlerFunc)
    m_messageHandlerFunc(mqMessage);

  //to encode output message
  std::ostringstream os;

  //get input message
  std::string msg((const char*)mqMessage.data(), mqMessage.size());
  std::istringstream is(msg);

  std::unique_ptr<DpaTask> dpaTask = m_factory.parseRequest(msg);
  if (dpaTask) {
    DpaTransactionTask trans(*dpaTask);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();
    os << dpaTask->encodeResponse(trans.getErrorStr());
  }
  else {
    os << m_factory.getLastError();
  }

  ustring msgu((unsigned char*)os.str().data(), os.str().size());
  sendMessage(msgu);
  //sendMessageToMqtt(os.str());

  return 1;
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

///////////////////
void Impl::setScheduler(IScheduler* scheduler)
{
  m_scheduler = scheduler;
  if (m_scheduler) {
    //m_scheduler->registerResponseHandler("mqtt", [&](const std::string& response) {
    //  ustring msgu((unsigned char*)response.data(), response.size());
    //  sendMessage(msgu);
    //});
    m_scheduler->registerResponseHandler("mqtt", [&](const ustring& response) {
      sendMessage(response);
    });
  }
}

void Impl::resetScheduler()
{
  //TODO
  //if (m_scheduler) {
  //  m_scheduler->unregisterResponseHandler("mqtt");
  //}
}
