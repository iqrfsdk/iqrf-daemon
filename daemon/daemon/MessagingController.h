#pragma once

#include "IClient.h"
#include "TaskQueue.h"
#include "IDaemon.h"
#include <map>
#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class IChannel;
class DpaHandler;

class MessagingController : public IDaemon
{
public:
  MessagingController(const std::string& iqrfPortName);
  virtual ~MessagingController();
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction);
  //TODO unregister
  virtual void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler);
  virtual std::set<IMessaging*>& getSetOfMessaging();
  virtual void registerMessaging(IMessaging& messaging);
  virtual void unregisterMessaging(IMessaging& messaging);
  virtual IScheduler* getScheduler() { return m_scheduler; }

  void startProtocols();
  void stopProtocols();

  void startClients();
  void stopClients();

  void start();
  void stop();
  void exit();
  void watchDog();

private:
  IChannel* m_iqrfInterface;
  DpaHandler* m_dpaHandler;

  void executeDpaTransactionFunc(DpaTransaction* dpaTransaction);

  TaskQueue<DpaTransaction*> *m_dpaTransactionQueue;
  std::string m_iqrfPortName;
  std::atomic_bool m_running;
  std::set<IMessaging*> m_protocols;
  
  std::map<std::string, IClient*> m_clients;

  IScheduler* m_scheduler;
  std::function<void(const DpaMessage&)> m_asyncHandler;
};
