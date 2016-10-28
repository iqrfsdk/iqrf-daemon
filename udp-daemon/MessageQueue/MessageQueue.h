#pragma once

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>

class MessageQueue
{
public:
  typedef std::function<void(const std::basic_string<unsigned char>&)> ProcessMessageFunc;

  MessageQueue(ProcessMessageFunc processMessageFunc);
  virtual ~MessageQueue();
  void pushToQueue(const std::basic_string<unsigned char>& message);
private:
  //thread function
  void worker();

  std::mutex m_messageQueueMutex, m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  std::queue<std::basic_string<unsigned char>> m_messageQueue;
  bool m_messagePushed;
  std::atomic_bool m_runWorkerThread;
  std::thread m_workerThread;

  ProcessMessageFunc m_processMessageFunc;
};
