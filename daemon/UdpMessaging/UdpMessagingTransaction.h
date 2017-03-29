#pragma once

#include "DpaTransaction.h"
#include "UdpChannel.h"
#include "IMessaging.h"
#include "TaskQueue.h"
#include "WatchDog.h"
#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class UdpMessaging;
class MessagingController;

class UdpMessagingTransaction : public DpaTransaction
{
public:
  UdpMessagingTransaction(UdpMessaging* udpMessaging);
  virtual ~UdpMessagingTransaction();
  const DpaMessage& getMessage() const override;
  int getTimeout() const override;
  void processConfirmationMessage(const DpaMessage& confirmation) override;
  void processResponseMessage(const DpaMessage& response) override;
  void processFinish(DpaRequest::DpaRequestStatus status) override;
  void setMessage(ustring message);
private:
  DpaMessage m_message;
  UdpMessaging* m_udpMessaging;
  bool m_success;
};

class UdpMessaging : public IMessaging
{
public:
  enum class Mode {
    Operational,
    Service,
    Forwarding
  };

  UdpMessaging() = delete;
  UdpMessaging(MessagingController* messagingController);
  virtual ~UdpMessaging();

  //component
  void start() override;
  void stop() override;
  void update(const rapidjson::Value& cfg) override;
  const std::string& getName() const override { return m_name; }

  //interface
  void registerMessageHandler(MessageHandlerFunc hndl) override;
  void unregisterMessageHandler() override;
  void sendMessage(const ustring& msg) override;

  void sendDpaMessageToUdp(const DpaMessage&  dpaMessage);

  void getGwIdent(ustring& message);
  void getGwStatus(ustring& message);

  int handleMessageFromUdp(const ustring& udpMessage);
  void encodeMessageUdp(ustring& udpMessage, const ustring& message = ustring());
  void decodeMessageUdp(const ustring& udpMessage, ustring& message);

  UdpMessaging::Mode parseMode(const std::string& mode);

private:
  void setExclusiveAccess();
  void resetExclusiveAccess();

  std::atomic_bool m_exclusiveAccess;
  WatchDog<std::function<void()>> m_watchDog;

  MessagingController *m_messagingController = nullptr;
  UdpMessagingTransaction* m_transaction = nullptr;

  UdpChannel *m_udpChannel;
  TaskQueue<ustring> *m_toUdpMessageQueue;

  //configuration
  std::string m_name;
  unsigned m_remotePort = 55000;
  unsigned m_localPort = 55300;
  Mode m_mode = Mode::Operational;
  unsigned m_timeout = 30000;
};
