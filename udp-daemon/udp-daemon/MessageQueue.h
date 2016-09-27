#pragma once

#include "Channel.h"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>

class MessageQueue
{
public:
  MessageQueue(Channel *channel);
  virtual ~MessageQueue();
  void pushToQueue(const std::basic_string<unsigned char>& message);

private:
  //thread function
  void sender();

  std::mutex m_messageQueueMutex, m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  std::queue<std::basic_string<unsigned char>> m_messageQueue;
  bool m_messagePushed;
  std::atomic_bool m_runSenderThread;
  std::thread m_senderThread;
  
  Channel *m_channel;
};
