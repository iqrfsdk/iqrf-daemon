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

#include "UdpMessagingTransaction.h"
#include "IDpaMessageForwarding.h"
#include "IDpaExclusiveAccess.h"
#include "UdpChannel.h"
#include "IMessaging.h"
#include "TaskQueue.h"
#include <string>
#include <atomic>

class DaemonController;

/// \class UdpMessaging
/// \brief UDP messaging
/// \details
/// Implements IMessaging interface for UDP communication
/// UdpMessaging works now as any IMessaging, but it is not used by any IService.
/// It behaves as IService, ISerializer and IMessaging together because of legacy reason.
/// It shall be changed in next version to keep architecture consistency with other components
///
/// Configurable via its update() method accepting JSON properties:
/// ```json
/// "Properties": {
///   "RemotePort": 55000,  #set remote port to send messages
///   "LocalPort" : 55300   #set local port to receive messages
/// }
/// ```
class UdpMessaging : public IMessaging, public IDpaMessageForwarding, public IDpaExclusiveAccess
{
public:
  /// \brief operational mode
  /// \details
  /// Operational is used for normal work
  /// Service the only UDP Messaging is used to communicate with IQRF IDE
  /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
  enum class Mode {
    Operational,
    Service,
    Forwarding
  };

  UdpMessaging() = delete;
  UdpMessaging(const std::string& name);

  virtual ~UdpMessaging();

  /// IMessaging overriden methods
  void start() override;
  void stop() override;
  void update(const rapidjson::Value& cfg) override;
  const std::string& getName() const override { return m_name; }
  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;

  // IDpaMessageForwarding overriden methods
  std::unique_ptr<DpaTransaction> getDpaTransactionForward(DpaTransaction* forwarded) override;

  // IDpaExclusiveAccess overriden methods
  void setExclusive(IChannel* chan) override;
  void resetExclusive() override;

  /// \brief Send DpaMessage to UDP
  /// \param [in] dpaMessage to send
  void sendDpaMessageToUdp(const DpaMessage&  dpaMessage);

  /// set daemon instance
  void setDaemon(IDaemon* d) { m_daemon = d; }

private:
  /// \brief Get GW identification for IQRF IDE
  /// \param [out] message composed message to send
  void getGwIdent(ustring& message);

  /// \brief Get GW status for IQRF IDE
  /// \param [out] message composed message to send
  void getGwStatus(ustring& message);

  /// \brief UDP message handler
  int handleMessageFromUdp(const ustring& udpMessage);
  
  /// \brief encode message
  /// \param [out] udpMessage encoded message
  /// \param [in] message to insert to udpMessage
  void encodeMessageUdp(ustring& udpMessage, const ustring& message = ustring());

  /// \brief decode message
  /// \param [in] udpMessage to decode
  /// \param [out] message content of udpMessage
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

  IDaemon *m_daemon = nullptr;
  UdpMessagingTransaction* m_operationalTransaction = nullptr;

  IChannel* m_exclusiveChannel = nullptr;
  UdpChannel* m_udpChannel = nullptr;
  IMessaging::MessageHandlerFunc m_messageHandlerFunc;
  TaskQueue<ustring> *m_toUdpMessageQueue = nullptr;

  // configuration
  std::string m_name;
  unsigned m_remotePort = 55000;
  unsigned m_localPort = 55300;
};
