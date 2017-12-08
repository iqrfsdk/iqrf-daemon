/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LaunchUtils.h"
#include "MqttMessaging.h"
#include "TaskQueue.h"
#include "MQTTAsync.h"
#include "PlatformDep.h"
#include "IDaemon.h"
#include "IqrfLogging.h"
#include <string.h>
#include <atomic>
#include <future>

INIT_COMPONENT(IMessaging, MqttMessaging)

class MqttMessagingImpl {

private:
  //configuration
  std::string m_mqttBrokerAddr;
  std::string m_mqttClientId;
  int m_mqttPersistence = 0;
  std::string m_mqttTopicRequest;
  std::string m_mqttTopicResponse;
  int m_mqttQos = 0;
  std::string m_mqttUser;
  std::string m_mqttPassword;
  bool m_mqttEnabledSSL = false;
  int m_mqttKeepAliveInterval = 20; //special msg sent to keep connection alive
  int m_mqttConnectTimeout = 5; //waits for accept from broker side
  int m_mqttMinReconnect = 1; //waits to reconnect when connection broken
  int m_mqttMaxReconnect = 64; //waits time *= 2 with every unsuccessful attempt up to this value 

  //The file in PEM format containing the public digital certificates trusted by the client.
  std::string m_trustStore;
  //The file in PEM format containing the public certificate chain of the client. It may also include
  //the client's private key.
  std::string m_keyStore;
  //If not included in the sslKeyStore, this setting points to the file in PEM format containing
  //the client's private key.
  std::string m_privateKey;
  //The password to load the client's privateKey if encrypted.
  std::string m_privateKeyPassword;
  //The list of cipher suites that the client will present to the server during the SSL handshake.For a
  //full explanation of the cipher list format, please see the OpenSSL on - line documentation :
  //http ://www.openssl.org/docs/apps/ciphers.html#CIPHER_LIST_FORMAT
  std::string m_enabledCipherSuites;
  //True/False option to enable verification of the server certificate
  bool m_enableServerCertAuth = true;

  
  std::string m_name;

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
  MqttMessagingImpl(const std::string& name)
    : m_toMqttMessageQueue(nullptr)
    , m_name(name)
  {
    m_connected = false;
  }

  //------------------------
  ~MqttMessagingImpl()
  {}

  //------------------------
  void update(const rapidjson::Value& cfg)
  {
    TRC_ENTER("");
    jutils::assertIsObject("", cfg);

    m_mqttBrokerAddr = jutils::getMemberAs<std::string>("BrokerAddr", cfg);
    m_mqttClientId = jutils::getMemberAs<std::string>("ClientId", cfg);
    m_mqttPersistence = jutils::getMemberAs<int>("Persistence", cfg);
    m_mqttQos = jutils::getMemberAs<int>("Qos", cfg);
    m_mqttTopicRequest = jutils::getMemberAs<std::string>("TopicRequest", cfg);
    m_mqttTopicResponse = jutils::getMemberAs<std::string>("TopicResponse", cfg);
    m_mqttUser = jutils::getMemberAs<std::string>("User", cfg);
    m_mqttPassword = jutils::getMemberAs<std::string>("Password", cfg);
    m_mqttEnabledSSL = jutils::getMemberAs<bool>("EnabledSSL", cfg);
    
    m_trustStore = jutils::getPossibleMemberAs<std::string>("TrustStore", cfg, m_trustStore);
    m_keyStore = jutils::getPossibleMemberAs<std::string>("KeyStore", cfg, m_keyStore);
    m_privateKey = jutils::getPossibleMemberAs<std::string>("PrivateKey", cfg, m_privateKey);
    m_privateKeyPassword = jutils::getPossibleMemberAs<std::string>("PrivateKeyPassword", cfg, m_privateKeyPassword);
    m_enabledCipherSuites = jutils::getPossibleMemberAs<std::string>("EnabledCipherSuites", cfg, m_enabledCipherSuites);
    m_enableServerCertAuth = jutils::getPossibleMemberAs<bool>("EnableServerCertAuth", cfg, m_enableServerCertAuth);

    m_mqttKeepAliveInterval = jutils::getPossibleMemberAs<int>("KeepAliveInterval", cfg, m_mqttKeepAliveInterval);
    m_mqttConnectTimeout = jutils::getPossibleMemberAs<int>("ConnectTimeout", cfg, m_mqttConnectTimeout);
    m_mqttMinReconnect = jutils::getPossibleMemberAs<int>("MinReconnect", cfg, m_mqttMinReconnect);
    m_mqttMaxReconnect = jutils::getPossibleMemberAs<int>("MaxReconnect", cfg, m_mqttMaxReconnect);

    TRC_LEAVE("");
  }

  //------------------------
  void start()
  {
    TRC_ENTER("");

    m_toMqttMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
      sendTo(msg);
    });

    m_ssl_opts.enableServerCertAuth = true;
    
    if (!m_trustStore.empty()) m_ssl_opts.trustStore = m_trustStore.c_str();
    if (!m_keyStore.empty()) m_ssl_opts.keyStore = m_keyStore.c_str();
    if (!m_privateKey.empty()) m_ssl_opts.privateKey = m_privateKey.c_str();
    if (!m_privateKeyPassword.empty()) m_ssl_opts.privateKeyPassword = m_privateKeyPassword.c_str();
    if (!m_enabledCipherSuites.empty()) m_ssl_opts.enabledCipherSuites = m_enabledCipherSuites.c_str();
    m_ssl_opts.enableServerCertAuth = m_enableServerCertAuth;

    int retval;

    if ((retval = MQTTAsync_create(&m_client, m_mqttBrokerAddr.c_str(),
      m_mqttClientId.c_str(), m_mqttPersistence, NULL)) != MQTTASYNC_SUCCESS) {
      THROW_EX(std::logic_error, "MQTTClient_create() failed: " << PAR(retval));
    }

    m_conn_opts.keepAliveInterval = m_mqttKeepAliveInterval;
    m_conn_opts.cleansession = 1;
    m_conn_opts.connectTimeout = m_mqttConnectTimeout;
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
      THROW_EX(std::logic_error, "MQTTClient_setCallbacks() failed: " << PAR(retval));
    }

    TRC_INF("daemon-MQTT-protocol started - trying to connect broker: " << m_mqttBrokerAddr);

    connect();

    TRC_LEAVE("");
  }

  //------------------------
  void stop()
  {
    TRC_ENTER("");

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

    TRC_INF("daemon-MQTT-protocol stopped");

    TRC_LEAVE("");
  }

  //------------------------
  const std::string& getName() const { return m_name; }

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
  void handleMessageFromMqtt(const ustring& mqMessage)
  {
    TRC_DBG("==================================" << std::endl <<
      "Received from MQTT: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

    if (m_messageHandlerFunc)
      m_messageHandlerFunc(mqMessage);
  }

  //------------------------
  void sendTo(const ustring& msg)
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
  void connectThread()
  {
    //TODO verify paho autoconnect and reuse if applicable
    int retval;
    int seconds = m_mqttMinReconnect;
    int seconds_max = m_mqttMaxReconnect;


    while (true) {
      TRC_DBG("Connecting: " << PAR(m_mqttBrokerAddr) << PAR(m_mqttClientId));
      if ((retval = MQTTAsync_connect(m_client, &m_conn_opts)) == MQTTASYNC_SUCCESS) {
      }
      else {
        TRC_WAR("MQTTAsync_connect() failed: " << PAR(retval));
      }

      // wait for connection result
      TRC_DBG("Going to sleep for: " << PAR(seconds));
      {
        std::unique_lock<std::mutex> lck(m_connectionMutex);
        if (m_connectionVariable.wait_for(lck, std::chrono::seconds(seconds),
          [this] {return m_connected == true || m_stopAutoConnect == true; }))
          break;
      }
      seconds = seconds < seconds_max ? seconds * 2 : seconds_max;
    }
  }

  //------------------------
  void connect()
  {
    TRC_ENTER("");
    int retval;

    m_stopAutoConnect = false;
    m_connected = false;
    m_subscribed = false;

    if (m_connectThread.joinable())
      m_connectThread.join();

    m_connectThread = std::thread([this]() { this->connectThread(); });
    TRC_LEAVE("");
  }


  //////////////////
  static void s_onConnect(void* context, MQTTAsync_successData* response) {
    ((MqttMessagingImpl*)context)->onConnect(response);
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
    ((MqttMessagingImpl*)context)->onConnectFailure(response);
  }
  void onConnectFailure(MQTTAsync_failureData* response) {
    TRC_ENTER("");
    if (response) {
      TRC_WAR("Connect failed: " << PAR(response->code) << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    }

    {
      std::unique_lock<std::mutex> lck(m_connectionMutex);
      m_connected = false;
      m_connectionVariable.notify_one();
    }
    TRC_LEAVE("");
  }

  //------------------------
  static void s_onSubscribe(void* context, MQTTAsync_successData* response) {
    ((MqttMessagingImpl*)context)->onSubscribe(response);
  }
  void onSubscribe(MQTTAsync_successData* response) {
    TRC_INF("Subscribe succeded: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    m_subscribed = true;
  }

  //------------------------
  static void s_onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    ((MqttMessagingImpl*)context)->onSubscribeFailure(response);
  }
  void onSubscribeFailure(MQTTAsync_failureData* response) {
    TRC_WAR("Subscribe failed: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos));
    m_subscribed = false;
  }

  //------------------------
  static void s_delivered(void *context, MQTTAsync_token dt) {
    ((MqttMessagingImpl*)context)->delivered(dt);
  }
  void delivered(MQTTAsync_token dt) {
    TRC_DBG("Message delivery confirmed" << PAR(dt));
    m_deliveredtoken = dt;
  }

  //------------------------
  static int s_msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    return ((MqttMessagingImpl*)context)->msgarrvd(topicName, topicLen, message);
  }
  int msgarrvd(char *topicName, int topicLen, MQTTAsync_message *message) {
    ustring msg((unsigned char*)message->payload, message->payloadlen);
    std::string topic;
    if (topicLen > 0)
      topic = std::string(topicName, topicLen);
    else
      topic = std::string(topicName);
    //TODO wildcards in comparison - only # supported now
    TRC_DBG(PAR(topic));
    size_t sz = m_mqttTopicRequest.size();
    if (m_mqttTopicRequest[--sz] == '#') {
      if (0 == m_mqttTopicRequest.compare(0, sz, topic, 0, sz))
        handleMessageFromMqtt(msg);
    }
    else if (0 == m_mqttTopicRequest.compare(topic))
      handleMessageFromMqtt(msg);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
  }

  //------------------------
  static void s_onSend(void* context, MQTTAsync_successData* response) {
    ((MqttMessagingImpl*)context)->onSend(response);
  }
  void onSend(MQTTAsync_successData* response) {
    TRC_DBG("Message sent successfuly");
  }

  //------------------------
  static void s_onSendFailure(void* context, MQTTAsync_failureData* response) {
    ((MqttMessagingImpl*)context)->onSendFailure(response);
  }
  void onSendFailure(MQTTAsync_failureData* response) {
    TRC_WAR("Message sent failure: " << PAR(response->code));
    //connect();
  }

  //------------------------
  static void s_connlost(void *context, char *cause) {
    ((MqttMessagingImpl*)context)->connlost(cause);
  }
  void connlost(char *cause) {
    TRC_WAR("Connection lost: " << NAME_PAR(cause, (cause ? cause : "nullptr")));
    connect();
  }

  //------------------------
  static void s_onDisconnect(void* context, MQTTAsync_successData* response) {
    ((MqttMessagingImpl*)context)->onDisconnect(response);
  }
  void onDisconnect(MQTTAsync_successData* response) {
    m_disconnect_promise.set_value(true);
  }

};

//////////////////
MqttMessaging::MqttMessaging(const std::string& name)
{
  m_impl = ant_new MqttMessagingImpl(name);
}

MqttMessaging::~MqttMessaging()
{
  delete m_impl;
}

void MqttMessaging::update(const rapidjson::Value& cfg)
{
  m_impl->update(cfg);
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
