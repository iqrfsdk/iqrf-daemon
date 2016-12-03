#pragma once

#include "DpaTransaction.h"
#include "IMessaging.h"
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

  virtual std::set<IMessaging*>& getSetOfMessaging() = 0;
  virtual void registerMessaging(IMessaging& messaging) = 0;
  virtual void unregisterMessaging(IMessaging& messaging) = 0;
  virtual IScheduler* getScheduler() = 0;
};
