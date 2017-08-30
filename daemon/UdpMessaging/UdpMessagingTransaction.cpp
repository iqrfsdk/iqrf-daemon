#include "UdpMessaging.h"
#include "IqrfLogging.h"

UdpMessagingTransaction::UdpMessagingTransaction(UdpMessaging* udpMessaging, DpaTransaction* forwarded)
  :m_udpMessaging(udpMessaging)
  ,m_forwarded(forwarded)
{
}

UdpMessagingTransaction::~UdpMessagingTransaction()
{
}

const DpaMessage& UdpMessagingTransaction::getMessage() const
{
  if (m_forwarded) {
    const DpaMessage& msg = m_forwarded->getMessage();
   // m_udpMessaging->sendDpaMessageToUdp(msg);
    return msg;
  }
  else {
    return m_message;
  }
}

int UdpMessagingTransaction::getTimeout() const
{
  if (m_forwarded) {
    return m_forwarded->getTimeout();
  }
  else {
    return -1;
  }
}

void UdpMessagingTransaction::processConfirmationMessage(const DpaMessage& confirmation)
{
  if (m_forwarded) {
    m_forwarded->processConfirmationMessage(confirmation);
  }
  //forward all
  m_udpMessaging->sendDpaMessageToUdp(confirmation);
}

void UdpMessagingTransaction::processResponseMessage(const DpaMessage& response)
{
  if (m_forwarded) {
    m_forwarded->processResponseMessage(response);
  }
  //forward all
  m_udpMessaging->sendDpaMessageToUdp(response);
}

void UdpMessagingTransaction::processFinish(DpaTransfer::DpaTransferStatus status)
{
  if (m_forwarded) {
    m_forwarded->processFinish(status);
  }

  m_success = (status == DpaTransfer::DpaTransferStatus::kAborted);
}

void UdpMessagingTransaction::setMessage(ustring message)
{
  m_message.DataToBuffer(message.data(), message.length());
}
