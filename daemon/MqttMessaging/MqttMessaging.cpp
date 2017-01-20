#include "MqttMessaging.h"
#include "TaskQueue.h"
#include "MQTTAsync.h"
#include "PlatformDep.h"
#include "IDaemon.h"
#include "IqrfLogging.h"
#include <string.h>
#include <atomic>
#include <future>

class Impl {

public:
  IDaemon* m_daemon;
private:
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

  std::string m_myName = "MqttMessaging";

  TaskQueue<ustring>* m_toMqttMessageQueue;
  IMessaging::MessageHandlerFunc m_messageHandlerFunc;

  MQTTAsync m_client;

  std::atomic<MQTTAsync_token> m_deliveredtoken;
  std::atomic_bool m_stopAutoConnect;
  std::atomic_bool m_connected;
  std::atomic_bool m_subscribed;

  std::thread m_connectThread;

  MQTTAsync_connectOptions m_conn_opts = MQTTAsync_connectOptions_initializer;
  MQTTAsync_SSLOptions m_ssl_opts = MQTTAsync_SSLOptions_initializer;
  MQTTAsync_disconnectOptions m_disc_opts = MQTTAsync_disconnectOptions_initializer;
  MQTTAsync_responseOptions m_subs_opts = MQTTAsync_responseOptions_initializer;
  MQTTAsync_responseOptions m_send_opts = MQTTAsync_responseOptions_initializer;

  std::mutex m_connectionMutex;
  std::condition_variable m_connectionVariable;

  std::promise<bool> m_disconnect_promise;
  std::future<bool> m_disconnect_future = m_disconnect_promise.get_future();

public:
  //------------------------
  Impl()
    :m_daemon(nullptr)
    , m_toMqttMessageQueue(nullptr)
  {
    m_connected = false;
  }

  //------------------------
  ~Impl()
  {}

  //------------------------
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
    m_mqttEnabledSSL = jutils::getMemberAs<bool>("EnabledSSL", cfg);

    TRC_LEAVE("");
  }

  //------------------------
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

    m_ssl_opts.enableServerCertAuth = true;
    int retval;

    if ((retval = MQTTAsync_create(&m_client, m_mqttBrokerAddr.c_str(),
      m_mqttClientId.c_str(), m_mqttPersistence, NULL)) != MQTTASYNC_SUCCESS) {
      THROW_EX(MqttChannelException, "MQTTClient_create() failed: " << PAR(retval));
    }

    m_conn_opts.keepAliveInterval = 20;
    m_conn_opts.cleansession = 1;
    m_conn_opts.connectTimeout = 5;
    m_conn_opts.username = m_mqttUser.c_str();
    m_conn_opts.password = m_mqttPassword.c_str();
    m_conn_opts.onSuccess = s_onConnect;
    m_conn_opts.onFailure = s_onConnectFailure;
    m_conn_opts.context = this;

    m_subs_opts.onSuccess = s_onSubscribe;
    m_subs_opts.onFailure = s_onSubscribeFailure;
    m_subs_opts.context = this;

    m_send_opts.onSuccess = s_onSend;
    m_send_opts.onFailure = s_onSendFailure;
    m_send_opts.context = this;

    if (m_mqttEnabledSSL) {
      m_conn_opts.ssl = &m_ssl_opts;
    }

    if ((retval = MQTTAsync_setCallbacks(m_client, this, s_connlost, s_msgarrvd, s_delivered)) != MQTTASYNC_SUCCESS) {
      THROW_EX(MqttChannelException, "MQTTClient_setCallbacks() failed: " << PAR(retval));
    }

    connect();

    std::cout << "daemon-MQTT-protocol started" << std::endl;

    TRC_LEAVE("");
  }

  //------------------------
  void Impl::stop()
  {
    TRC_ENTER("");
    if (m_enabled) {

      ///stop possibly running connect thread
      m_stopAutoConnect = true;
      onConnectFailure(nullptr);
      if (m_connectThread.joinable())
        m_connectThread.join();

      int retval;
      m_disc_opts.onSuccess = s_onDisconnect;
      m_disc_opts.context = this;
      if ((retval = MQTTAsync_disconnect(m_client, &m_disc_opts)) != MQTTASYNC_SUCCESS) {
        TRC_WAR("Failed to start disconnect: " << PAR(retval));
        onDisconnect(nullptr);
      }

      //wait for async disconnect
      std::chrono::milliseconds span(5000);
      if (m_disconnect_future.wait_for(span) == std::future_status::timeout) {
        TRC_WAR("Timeout to wait disconnect");
      }

      MQTTAsync_destroy(&m_client);
      delete m_toMqttMessageQueue;
      std::cout << "daemon-MQTT-protocol stopped" << std::endl;
    }
    TRC_LEAVE("");
  }

  //------------------------
  const std::string& getName() const { return m_myName; }

  //------------------------
  void registerMessageHandler(IMessaging::MessageHandlerFunc hndl) {
    m_messageHandlerFunc = hndl;
  }

  //------------------------
  void unregisterMessageHandler() {
    m_messageHandlerFunc = IMessaging::MessageHandlerFunc();
  }

  //------------------------
  void sendMessage(const ustring& msg) {
    m_toMqttMessageQueue->pushToQueue(msg);
  }

  //------------------------
  void Impl::handleMessageFromMqtt(const ustring& mqMessage)
  {
    TRC_DBG("==================================" << std::endl <<
      "Received from MQTT: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

    if (m_messageHandlerFunc)
      m_messageHandlerFunc(mqMessage);
  }

  //------------------------
  void Impl::sendTo(const ustring& msg)
  {
    TRC_DBG("Sending to MQTT: " << NAME_PAR(topic, m_mqttTopicResponse) << std::endl <<
      FORM_HEX(msg.data(), msg.size()));

    if (m_connected) {

      int retval;
      MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

      pubmsg.payload = (void*)msg.data();
      pubmsg.payloadlen = (int)msg.size();
      pubmsg.qos = m_mqttQos;
      pubmsg.retained = 0;

      m_deliveredtoken = 0;

      if ((retval = MQTTAsync_sendMessage(m_client, m_mqttTopicResponse.c_str(), &pubmsg, &m_send_opts)) != MQTTASYNC_SUCCESS)
      {
        TRC_WAR("Failed to start sendMessage: " << PAR(retval));
      }

    }
    else {
      TRC_WAR("Cannot send to MQTT: connection lost");
    }
  }

  //------------------------
  void Impl::connectThread()
  {
    int retval;
    while (!m_connected && !m_stopAutoConnect) {
      TRC_DBG("Connecting: " << PAR(m_mqttBrokerAddr) << PAR(m_mqttClientId));
      if ((retval = MQTTAsync_connect(m_client, &m_conn_opts)) == MQTTASYNC_SUCCESS) {
      }
      else {
        TRC_WAR("MQTTAsync_connect() failed: " << PAR(retval));
      }

      // wait for connection result
      {
        std::unique_lock<std::mutex> lck(m_connectionMutex);
        if (!m_connected)
          m_connectionVariable.wait(lck);
      }
    }
  }

  //------------------------
  void Impl::connect()
  {
    int retval;

    m_stopAutoConnect = false;
    m_connected = false;
    m_subscribed = false;

    if (m_connectThread.joinable())
      m_connectThread.join();

    m_connectThread = std::thread([this]() { this->connectThread(); });
  }


  //////////////////
  static void s_onConnect(void* context, MQTTAsync_successData* response) {
    ((Impl*)context)->onConnect(response);
  }
  void onConnect(MQTTAsync_successData* response) {
    TRC_INF("Connect succeded: " << PAR(m_mqttBrokerAddr) << PAR(m_mqttClientId));

    {
      std::unique_lock<std::mutex> lck(m_connectionMutex);
      m_connected = true;
      m_connectionVariable.notify_one();
    }

    int retval;
    TRC_DBG("Subscribing: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    if ((retval = MQTTAsync_subscribe(m_client, m_mqttTopicRequest.c_str(), m_mqttQos, &m_subs_opts)) != MQTTASYNC_SUCCESS) {
      TRC_WAR("MQTTAsync_subscribe() failed: " << PAR(retval) << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    }
  }

  //------------------------
  static void s_onConnectFailure(void* context, MQTTAsync_failureData* response) {
    ((Impl*)context)->onConnectFailure(response);
  }
  void onConnectFailure(MQTTAsync_failureData* response) {
    if (response) {
      TRC_WAR("Connect failed: " << PAR(response->code) << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    }

    {
      std::unique_lock<std::mutex> lck(m_connectionMutex);
      m_connected = false;
      m_connectionVariable.notify_one();
    }
  }

  //------------------------
  static void s_onSubscribe(void* context, MQTTAsync_successData* response) {
    ((Impl*)context)->onSubscribe(response);
  }
  void onSubscribe(MQTTAsync_successData* response) {
    TRC_INF("Subscribe succeded: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    m_subscribed = true;
  }

  //------------------------
  static void s_onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    ((Impl*)context)->onSubscribeFailure(response);
  }
  void onSubscribeFailure(MQTTAsync_failureData* response) {
    TRC_WAR("Subscribe failed: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    m_subscribed = false;
  }

  //------------------------
  static void s_delivered(void *context, MQTTAsync_token dt) {
    ((Impl*)context)->delivered(dt);
  }
  void delivered(MQTTAsync_token dt) {
    TRC_DBG("Message delivery confirmed" << PAR(dt));
    m_deliveredtoken = dt;
  }

  //------------------------
  static int s_msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    return ((Impl*)context)->msgarrvd(topicName, topicLen, message);
  }
  int msgarrvd(char *topicName, int topicLen, MQTTAsync_message *message) {
    ustring msg((unsigned char*)message->payload, message->payloadlen);
    if (!strncmp(topicName, m_mqttTopicRequest.c_str(), topicLen))
      handleMessageFromMqtt(msg);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
  }

  //------------------------
  static void s_onSend(void* context, MQTTAsync_successData* response) {
    ((Impl*)context)->onSend(response);
  }
  void onSend(MQTTAsync_successData* response) {
    TRC_DBG("Message sent successfuly");
  }

  //------------------------
  static void s_onSendFailure(void* context, MQTTAsync_failureData* response) {
    ((Impl*)context)->onSendFailure(response);
  }
  void onSendFailure(MQTTAsync_failureData* response) {
    TRC_WAR("Message sent failure: " << PAR(response->code));
    connect();
  }

  //------------------------
  static void s_connlost(void *context, char *cause) {
    ((Impl*)context)->connlost(cause);
  }
  void connlost(char *cause) {
    TRC_WAR("Connection lost: " << NAME_PAR(cause, (cause ? cause : "nullptr")));
    connect();
  }

  //------------------------
  static void s_onDisconnect(void* context, MQTTAsync_successData* response) {
    ((Impl*)context)->onDisconnect(response);
  }
  void onDisconnect(MQTTAsync_successData* response) {
    m_disconnect_promise.set_value(true);
  }

};

//////////////////
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
