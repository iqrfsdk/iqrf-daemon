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

#include "DpaTransaction.h"
#include <string>

typedef std::basic_string<unsigned char> ustring;

class UdpMessaging;

class UdpMessagingTransaction : public DpaTransaction
{
public:
  UdpMessagingTransaction(UdpMessaging* udpMessaging, DpaTransaction* forwarded = nullptr);
  virtual ~UdpMessagingTransaction();
  const DpaMessage& getMessage() const override;
  int getTimeout() const override;
  void processConfirmationMessage(const DpaMessage& confirmation) override;
  void processResponseMessage(const DpaMessage& response) override;
  void processFinish(DpaTransfer::DpaTransferStatus status) override;
  void setMessage(ustring message);
private:
  DpaMessage m_message;
  UdpMessaging* m_udpMessaging;
  bool m_success;
  DpaTransaction* m_forwarded;
};
