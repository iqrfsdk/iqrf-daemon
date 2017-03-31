#pragma once

#include "DpaTransaction.h"
#include <string>

typedef std::basic_string<unsigned char> ustring;

class UdpMessaging;

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
  DpaTransaction* m_sniffedDpaTransaction = nullptr;
};
