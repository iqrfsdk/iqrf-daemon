#pragma once

#include "TaskQueue.h"
#include "IDaemon.h"
#include <unordered_map>
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
  virtual std::set<IMessaging*>& getSetOfMessaging();
  virtual void registerMessaging(IMessaging& messaging);
  virtual void unregisterMessaging(IMessaging& messaging);

  void startProtocols();
  void stopProtocols();

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
  
  //<pacid_to_IQRF, pair<pacid_to_protocol, messaging*>>
  std::unordered_map<unsigned short, std::pair<unsigned short, IMessaging*>> m_pacid_mapping;

};
