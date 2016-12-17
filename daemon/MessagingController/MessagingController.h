#pragma once

#include "ISerializer.h"
#include "IMessaging.h"
#include "IClient.h"
#include "TaskQueue.h"
#include "IDaemon.h"
#include "JsonUtils.h"
#include <map>
#include <string>
#include <atomic>

typedef std::basic_string<unsigned char> ustring;

class IChannel;
class DpaHandler;

class MessagingController : public IDaemon
{
public:
  MessagingController(const std::string& cfgFileName);
  virtual ~MessagingController();
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction);
  //TODO unregister
  virtual void registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> message_handler);
  virtual std::set<IMessaging*>& getSetOfMessaging();
  virtual void registerMessaging(IMessaging& messaging);
  virtual void unregisterMessaging(IMessaging& messaging);
  virtual IScheduler* getScheduler() { return m_scheduler; }

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
  std::atomic_bool m_running;
  std::set<IMessaging*> m_protocols;
  
  std::map<std::string, IClient*> m_clients;
  std::vector<ISerializer*> m_serializers;
  std::vector<IMessaging*> m_messagings;

  IScheduler* m_scheduler;
  std::function<void(const DpaMessage&)> m_asyncHandler;

  //configuration
  rapidjson::Document m_configuration;
  std::string m_cfgFileName;
  const std::string m_cfgVersion = "v0.0";
  std::string m_traceFileName;
  int m_traceFileSize = 0;
  std::string m_iqrfInterfaceName;
};
