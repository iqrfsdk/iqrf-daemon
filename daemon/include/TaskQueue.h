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
      std::unique_lock<std::mutex> lck(m_taskQueueMutex);
      m_runWorkerThread = false;
      m_taskPushed = true;
    }
    m_conditionVariable.notify_all();

    if (m_workerThread.joinable()) {
      m_workerThread.join();
    }
  }

  int pushToQueue(const T& task)
  {
    int retval = 0;
    {
      std::unique_lock<std::mutex> lck(m_taskQueueMutex);
      m_taskQueue.push(task);
      retval = m_taskQueue.size();
      m_taskPushed = true;
    }
    m_conditionVariable.notify_all();
    return retval;
  }

private:
  //thread function
  void worker()
  {
    std::unique_lock<std::mutex> lck(m_taskQueueMutex, std::defer_lock);

    while (m_runWorkerThread) {

      //wait for something in the queue
      lck.lock();
      m_conditionVariable.wait(lck, [&] { return m_taskPushed; }); //lock is released in wait
      //lock is reacquired here
      m_taskPushed = false;

      while (true) {
        if (!m_taskQueue.empty()) {
          auto task = m_taskQueue.front();
          m_taskQueue.pop();
          lck.unlock();
          m_processTaskFunc(task);
        } else {
          lck.unlock();
          break;
        }
        lck.lock(); //lock for next iteration
      }
    }
  }

  std::mutex m_taskQueueMutex;
  std::condition_variable m_conditionVariable;
  std::queue<T> m_taskQueue;
  bool m_taskPushed;
  bool m_runWorkerThread;
  std::thread m_workerThread;

  ProcessTaskFunc m_processTaskFunc;
};
