/*
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

////////////////////////////////
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>

/// \class WatchDog
/// \brief Provides watch dog feature
/// \details
/// It starts dedicated monitor thread and waits for calling pet(). If pet() is not called at predefined time interval
/// it calls callback. 
template <typename T>
class WatchDog
{
public:
  WatchDog() {
  };

  /// \brief parametric constructor
  /// \param [in] millis time to wait for pet()
  /// \param [in] callback functional called when time without pet() elapsed
  /// \details
  /// Construc and start watching thread. If pet() is not called at predefined time it calls callback. 
  WatchDog(unsigned int millis, T callback) {
    start(millis, callback);
  }

  virtual ~WatchDog() {
    stop();
  }

  /// \brief Start watching thread
  /// \param [in] millis time to wait for pet()
  /// \param [in] callback functional called when time without pet() elapsed
  /// \details
  /// Start watching thread if not running yet. If pet() is not called at predefined time it calls callback. 
  void start(unsigned int millis, T callback) {
    std::unique_lock<std::mutex> lock(m_stopConditionMutex);
    if (!m_running) {
      m_running = true;
      m_callback = callback;
      m_timeout = std::chrono::milliseconds(millis);
      m_lastRefreshTime = std::chrono::system_clock::now();
      if (m_thread.joinable()) m_thread.join();
      m_running = true;
      m_thread = std::thread(&WatchDog::watch, this);
    }
  }

  /// \brief Stop watching thread
  /// \details
  /// Stop watching thread if running
  void stop() {
    {
      std::unique_lock<std::mutex> lock(m_stopConditionMutex);
      if (m_running) {
        m_running = false;
        m_stopConditionVariable.notify_all();
      }
    }
    if (m_thread.joinable())
      m_thread.join();
  }

  /// \brief Pet watch dog
  /// \details
  /// Satisfy watch dog for time interval
  void pet() {
    std::unique_lock<std::mutex> lock(m_stopConditionMutex);
    m_lastRefreshTime = std::chrono::system_clock::now();
  }

private:
  /// Watching thread
  void watch() {
    {
      std::unique_lock<std::mutex> lock(m_stopConditionMutex);
      while (m_running && ((std::chrono::system_clock::now() - m_lastRefreshTime)) < m_timeout)
        m_stopConditionVariable.wait_for(lock, m_timeout);
    }
    {
      std::unique_lock<std::mutex> lock(m_stopConditionMutex);
      if (m_running) {
        m_running = false;
        m_callback();
      }
    }
  }

  bool m_running = false;
  std::thread m_thread;
  T m_callback;
  std::mutex m_stopConditionMutex;
  std::condition_variable m_stopConditionVariable;
  std::chrono::system_clock::time_point m_lastRefreshTime;
  std::chrono::milliseconds m_timeout;

};
