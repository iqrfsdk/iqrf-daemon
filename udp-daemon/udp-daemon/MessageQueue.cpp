#include "MessageQueue.h"

MessageQueue::MessageQueue(Channel *channel)
  :m_channel(channel)
{
  m_messagePushed = false;
  m_runSenderThread = true;
  m_senderThread = std::thread(&MessageQueue::sender, this);
}

MessageQueue::~MessageQueue()
{
  m_runSenderThread = false;
  std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
  m_messagePushed = true;
  m_conditionVariable.notify_one();

  if (m_senderThread.joinable())
    m_senderThread.join();
}

void MessageQueue::pushToQueue(const std::basic_string<unsigned char>& message)
{
  {
    std::lock_guard<std::mutex> lck(m_messageQueueMutex);
    m_messageQueue.push(message);
  }
  std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
  m_messagePushed = true;
  m_conditionVariable.notify_one();
}

void MessageQueue::sender()
{
  while (m_runSenderThread) {

    { //wait for something in the queue
      std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
      m_conditionVariable.wait(lck, [&]{ return m_messagePushed; });
      m_messagePushed = false;
    }

    while (true) {
      std::lock_guard<std::mutex> lck(m_messageQueueMutex);
      if (!m_messageQueue.empty()) {
        m_channel->sendTo(m_messageQueue.front());
        m_messageQueue.pop();
      }
      else {
        break;
      }
    }
  }
}
