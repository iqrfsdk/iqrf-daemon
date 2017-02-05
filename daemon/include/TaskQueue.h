#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>

template <class T>
class TaskQueue
{
public:
  typedef std::function<void(T)> ProcessTaskFunc;

  TaskQueue(ProcessTaskFunc processTaskFunc)
    :m_processTaskFunc(processTaskFunc)
  {
    m_taskPushed = false;
    m_runWorkerThread = true;
    m_workerThread = std::thread(&TaskQueue::worker, this);
  }

  virtual ~TaskQueue()
  {
    {
      m_runWorkerThread = false;
      std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
      m_taskPushed = true;
      m_conditionVariable.notify_one();
    }

    if (m_workerThread.joinable())
      m_workerThread.join();
  }

  void pushToQueue(const T& task)
  {
    {
      std::lock_guard<std::mutex> lck(m_taskQueueMutex);
      m_taskQueue.push(task);
    }
    {
      std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
      m_taskPushed = true;
      m_conditionVariable.notify_one();
    }
  }

private:
  //thread function
  void worker()
  {
    while (m_runWorkerThread) {

      { //wait for something in the queue
        std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
        m_conditionVariable.wait(lck, [&]{ return m_taskPushed; });
        m_taskPushed = false;
      }

      while (true) {
        m_taskQueueMutex.lock();
        if (!m_taskQueue.empty()) {
          T toBeProcessed = m_taskQueue.front();
          m_taskQueue.pop();
          m_taskQueueMutex.unlock();
          m_processTaskFunc(toBeProcessed);
        }
        else {
          m_taskQueueMutex.unlock();
          break;
        }
      }
    }
  }

  std::mutex m_taskQueueMutex, m_conditionVariableMutex;
  std::condition_variable m_conditionVariable;
  std::queue<T> m_taskQueue;
  bool m_taskPushed;
  std::atomic_bool m_runWorkerThread;
  std::thread m_workerThread;

  ProcessTaskFunc m_processTaskFunc;
};
