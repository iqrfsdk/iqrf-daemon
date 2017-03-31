#include "UdpMessaging.h"
#include "IqrfLogging.h"

UdpMessagingTransaction::UdpMessagingTransaction(UdpMessaging* udpMessaging)
  :m_udpMessaging(udpMessaging)
{
}

UdpMessagingTransaction::~UdpMessagingTransaction()
{
}

const DpaMessage& UdpMessagingTransaction::getMessage() const
{
  if (m_sniffedDpaTransaction) {
    return m_sniffedDpaTransaction->getMessage();
  }
  else {
    return m_message;
  }
}

int UdpMessagingTransaction::getTimeout() const
{
  if (m_sniffedDpaTransaction) {
    return m_sniffedDpaTransaction->getTimeout();
  }
  else {
    return -1;
  }
}

void UdpMessagingTransaction::processConfirmationMessage(const DpaMessage& confirmation)
{
  if (m_sniffedDpaTransaction) {
    m_sniffedDpaTransaction->processConfirmationMessage(confirmation);
  }
  //forward all
  m_udpMessaging->sendDpaMessageToUdp(confirmation);
}

void UdpMessagingTransaction::processResponseMessage(const DpaMessage& response)
{
  if (m_sniffedDpaTransaction) {
    m_sniffedDpaTransaction->processResponseMessage(response);
  }
  //forward all
  m_udpMessaging->sendDpaMessageToUdp(response);
}

void UdpMessagingTransaction::processFinish(DpaRequest::DpaRequestStatus status)
{
  if (m_sniffedDpaTransaction) {
    m_sniffedDpaTransaction->processFinish(status);
  }

  m_success = isProcessed(status);
}

void UdpMessagingTransaction::setMessage(ustring message)
{
  m_message.DataToBuffer(message.data(), message.length());
}
