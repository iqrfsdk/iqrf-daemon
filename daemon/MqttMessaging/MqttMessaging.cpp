#include "MqttMessaging.h"
#include "DpaTransactionTask.h"

#include "PlatformDep.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

const std::string MQ_ERROR_ADDRESS("ERROR_ADDRESS");
const std::string MQ_ERROR_DEVICE("ERROR_DEVICE");

//#include "MqChannel.h"
////////
#include "MQTTClient.h"
//#define ADDRESS     "tcp://localhost:1883"
#define ADDRESS     "tcp://192.168.1.26:1883"
#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "MQTT Examples"
#define TOPIC       "hello/world"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
  printf("Message with token value %d delivery confirmed\n", dt);
  deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
  int i;
  char* payloadptr;

  printf("Message arrived\n");
  printf("     topic: %s\n", topicName);
  printf("   message: ");

  payloadptr = (char*)message->payload;
  for (i = 0; i<message->payloadlen; i++)
  {
    putchar(*payloadptr++);
  }
  putchar('\n');
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);
  return 1;
}

void connlost(void *context, char *cause)
{
  printf("\nConnection lost\n");
  printf("     cause: %s\n", cause);
}
////////

MqttMessaging::MqttMessaging()
  :m_daemon(nullptr)
  //, m_mqChannel(nullptr)
  , m_toMqttMessageQueue(nullptr)
{
  //m_localMqName = "iqrf-daemon-110";
  //m_remoteMqName = "iqrf-daemon-100";
}

MqttMessaging::~MqttMessaging()
{
}

void MqttMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

void MqttMessaging::start()
{
  TRC_ENTER("");

  //m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqttMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    //m_mqChannel->sendTo(msg);
  });

  //m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
  //  return handleMessageFromMq(msg); });

  //////////////////
  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  int rc;
  int ch;

  MQTTClient_create(&client, ADDRESS, CLIENTID,
    MQTTCLIENT_PERSISTENCE_NONE, NULL);
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;

  MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

  if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", TOPIC, CLIENTID, QOS);
  MQTTClient_subscribe(client, TOPIC, QOS);
  //////////////////

  std::cout << "daemon-MQTT-protocol started" << std::endl;

  TRC_LEAVE("");
}

void MqttMessaging::stop()
{
  TRC_ENTER("");
  //delete m_mqChannel;
  delete m_toMqttMessageQueue;
  std::cout << "daemon-MQTT-protocol stopped" << std::endl;
  TRC_LEAVE("");
}

void MqttMessaging::sendMessageToMqtt(const std::string& message)
{
  ustring msg((unsigned char*)message.data(), message.size());
  TRC_DBG(FORM_HEX(message.data(), message.size()));
  m_toMqttMessageQueue->pushToQueue(msg);
}

int MqttMessaging::handleMessageFromMqtt(const ustring& mqMessage)
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
