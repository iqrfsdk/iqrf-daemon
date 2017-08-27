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
#include "IService.h"
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
  virtual IScheduler* getScheduler() = 0;
  virtual std::string doCommand(const std::string& cmd) = 0;
  virtual const std::string& getModuleId() = 0;
  virtual const std::string& getOsVersion() = 0;
  virtual const std::string& getTrType() = 0;
  virtual const std::string& getMcuType() = 0;
  virtual const std::string& getOsBuild() = 0;
  virtual const std::string& getDaemonVersion() = 0;
  virtual const std::string& getDaemonVersionBuild() = 0;
};
