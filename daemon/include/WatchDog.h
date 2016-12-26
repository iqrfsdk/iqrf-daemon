#pragma once

////////////////////////////////
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>

template <typename T>
class WatchDog
{
public:
  WatchDog() {
  };

  WatchDog(unsigned int millis, T callback) {
    start(millis, callback);
  }

  virtual ~WatchDog() {
    stop();
  }

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

  void pet() {
    std::unique_lock<std::mutex> lock(m_stopConditionMutex);
    m_lastRefreshTime = std::chrono::system_clock::now();
  }

private:
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
  //unsigned int m_timeout = 100;
  std::chrono::milliseconds m_timeout;

};
