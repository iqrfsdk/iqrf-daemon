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

#pragma once

#include "JsonUtils.h"
#include "IMessaging.h"
#include <string>

class MqttMessagingImpl;

typedef std::basic_string<unsigned char> ustring;

/// \class MqttMessaging
/// \brief MQTT messaging
/// \details
/// Implements IMessaging interface for MQTT communication by MQTT Paho library
///
/// Configurable via its update() method accepting JSON properties:
/// ```json
/// "Properties": {
///   "BrokerAddr": "tcp://127.0.0.1:1883",   #broker address
///   "ClientId": "IqrfDpaMessaging1",        #unique instance name
///   "Persistence": 1,                       #MQTT persistence value
///   "Qos": 1,                               #MQTT QoS value 
///   "TopicRequest": "Iqrf/DpaRequest",      #MQTT topic expected for incoming messages
///   "TopicResponse": "Iqrf/DpaResponse",    #MQTT topic used for outgoing messages
///   "User": "",                             #MQTT user for authentication
///   "Password": "",                         #MQTT password for authentication
///   "EnabledSSL": false,                    #MQTT SSL
///   "KeepAliveInterval": 20,                #Paho keep alive interval value
///   "ConnectTimeout": 5,                    #Paho connect timeout value
///   "MinReconnect": 1,                      #Paho minimal reconnect value
///   "MaxReconnect": 64,                     #Paho maximal reconnect value
///   "TrustStore": "server-ca.crt",          #SSL parameter
///   "KeyStore": "client.pem",               #SSL parameter
///   "PrivateKey": "client-privatekey.pem",  #SSL parameter
///   "PrivateKeyPassword": "",               #SSL parameter
///   "EnabledCipherSuites": "",              #SSL parameter
///   "EnableServerCertAuth": true            #SSL parameter
/// }
/// ```
class MqttMessaging : public IMessaging
{
public:
  MqttMessaging() = delete;
  MqttMessaging(const std::string& name);

  virtual ~MqttMessaging();

  /// IMessaging overriden methods
  void start() override;
  void stop() override;
  void update(const rapidjson::Value& cfg) override;
  const std::string& getName() const override;
  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;

private:
  MqttMessagingImpl* m_impl;
};
