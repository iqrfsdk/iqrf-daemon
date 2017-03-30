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
#include "IMessaging.h"
#include "IClient.h"
#include <set>
#include <string>
#include <functional>

class IScheduler;

class IDaemon
{
public:
  virtual ~IDaemon() {};
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction) = 0;
  virtual void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler) = 0;

  //virtual std::set<IMessaging*>& getSetOfMessaging() = 0;
  //virtual void registerMessaging(IMessaging& messaging) = 0;
  //virtual void unregisterMessaging(IMessaging& messaging) = 0;
  //virtual void registerClientService(IClient& cs) = 0;
  //virtual void unregisterClientService(IClient& cs) = 0;
  virtual IScheduler* getScheduler() = 0;
};
