#include "MqttMessaging.h"
#include "TaskQueue.h"
#include "DpaTransactionTask.h"
#include "MQTTClient.h"
#include "PlatformDep.h"
#include "IDaemon.h"
#include "IqrfLogging.h"
#include <atomic>

const std::string MQ_ERROR_ADDRESS("ERROR_ADDRESS");
const std::string MQ_ERROR_DEVICE("ERROR_DEVICE");

const std::string MQTT_BROKER_ADDRESS("tcp://192.168.1.26:1883");
const std::string MQTT_CLIENTID("IqrfDpaMessaging");
const std::string MQTT_TOPIC("Iqrf/Dpa");
const int MQTT_QOS(1);
//#define MQTT_TIMEOUT     10000L

//volatile MQTTClient_deliveryToken deliveredtoken;

class Impl {
public:
  Impl::Impl()
    :m_daemon(nullptr)
    ,m_toMqttMessageQueue(nullptr)
  {}

  Impl::~Impl()
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

  void delivered(MQTTClient_deliveryToken dt)
  {
    printf("Message with token value %d delivery confirmed\n", dt);
    m_deliveredtoken = dt;
  }

  int msgarrvd(char *topicName, int topicLen, MQTTClient_message *message)
  {
    ustring msg((unsigned char*)message->payload, message->payloadlen);
    if (!strncmp(topicName, MQTT_TOPIC.c_str(), topicLen))
      handleMessageFromMqtt(msg);
    return 1;
  }

  void connlost(char *cause)
  {
    TRC_WAR("Connection lost: " << PAR(cause));
    m_connected = false;
  }

  IDaemon* m_daemon;
  //MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqttMessageQueue;
  std::atomic_bool m_connected;
  std::atomic<MQTTClient_deliveryToken> m_deliveredtoken;
  MQTTClient m_client;
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

void Impl::start()
{
  TRC_ENTER("");

  //m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqttMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    //m_mqChannel->sendTo(msg);
  });

  m_client = ant_new MQTTClient();

  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  int retval;

  MQTTClient_create(&m_client, MQTT_BROKER_ADDRESS.c_str(), MQTT_CLIENTID.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;

  MQTTClient_setCallbacks(m_client, this, sconnlost, smsgarrvd, sdelivered);

  if ((retval = MQTTClient_connect(m_client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    THROW_EX(MqttChannelException, "MQTTClient_connect() failed: " << PAR(retval));
  }

  TRC_DBG("Subscribing: " << PAR(MQTT_TOPIC) << PAR(MQTT_CLIENTID) << PAR(MQTT_QOS));
  if ((retval = MQTTClient_subscribe(m_client, MQTT_TOPIC.c_str(), MQTT_QOS)) != MQTTCLIENT_SUCCESS) {
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
  delete m_client;
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

  sendMessageToMqtt(os.str());

  return 0;
}
