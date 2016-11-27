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

  static void sdelivered(void *context, MQTTClient_deliveryToken dt) {
    ((Impl*)context)->delivered(dt);
  }

  static int smsgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    return ((Impl*)context)->msgarrvd(topicName, topicLen, message);
  }

  static void sconnlost(void *context, char *cause) {
    ((Impl*)context)->connlost(cause);
  }

  void sendMessageToMqtt(const std::string& message);
  int handleMessageFromMqtt(const ustring& mqMessage);
  int handleMessageFromMqtt1(const ustring& mqMessage);

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
  //MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqttMessageQueue;
  std::atomic_bool m_connected;
  std::atomic<MQTTClient_deliveryToken> m_deliveredtoken;
  MQTTClient m_client;
  
  IScheduler* m_scheduler;
  DpaTaskSimpleSerializerFactory m_factory;
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

  MQTTClient_setCallbacks(m_client, this, sconnlost, smsgarrvd, sdelivered);

  if ((retval = MQTTClient_connect(m_client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_connect() failed: " << PAR(retval));
  }

  TRC_DBG("Subscribing: " << PAR(MQTT_TOPIC_DPA_REQUEST) << PAR(MQTT_CLIENTID) << PAR(MQTT_QOS));
  if ((retval = MQTTClient_subscribe(m_client, MQTT_TOPIC_DPA_REQUEST.c_str(), MQTT_QOS)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_subscribe() failed: " << PAR(retval));
  }

  m_connected = true;

  /////////////////////
  // register
  //m_factory.registerClass<>(
  /////////////////////

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

void Impl::sendMessageToMqtt(const std::string& message)
{
  ustring msg((unsigned char*)message.data(), message.size());
  TRC_DBG(FORM_HEX(message.data(), message.size()));
  m_toMqttMessageQueue->pushToQueue(msg);
}

////////////////////////////////////
////////////////////////////////////
////////////////////////////////////
int Impl::handleMessageFromMqtt(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQTT: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  //to encode output message
  std::ostringstream os;

  //get input message
  std::string msg((const char*)mqMessage.data(), mqMessage.size());
  std::istringstream is(msg);

  std::unique_ptr<DpaTask> dpaTask = m_factory.parse(msg);
  if (dpaTask) {
    //PrfThermometer temp(address, PrfThermometer::Cmd::READ);
    DpaTransactionTask trans(*dpaTask);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();
    dpaTask->encodeResponseMessage(os, trans.getErrorStr());
  }
  else {
    os << MQ_ERROR_DEVICE;
  }

  sendMessageToMqtt(os.str());

  return 1;
}

#if 0
int Impl::handleMessageFromMqtt(const ustring& mqMessage)
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
    sendMessageToMqtt(MQ_ERROR_ADDRESS);
    return -1;
  }

  //to encode output message
  std::ostringstream os;

  if (device == PrfThermometer::PRF_NAME_Thermometer) {
    PrfThermometer temp((int)address, (int)PrfThermometer::Cmd::READ);
    DpaTransactionTask trans(temp);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << temp;
    else
      os << trans.getErrorStr();
  }
  else if (device == PrfLedG::PRF_NAME_LedG) {
    PrfLedG pulse((int)address, (int)PrfLed::Cmd::PULSE);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else if (device == PrfLedR::PRF_NAME_LedR) {
    PrfLedR pulse((int)address, (int)PrfLed::Cmd::PULSE);
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

  sendMessageToMqtt(os.str());

  return 0;
}
#endif

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
    m_scheduler->registerResponseHandler("mqtt", [&](const std::string& response) {
      sendMessageToMqtt(response);
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
