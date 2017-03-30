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

#include <string>
#include <functional>
#include <vector>
#include <chrono>

class IScheduler
{
public:
  typedef long TaskHandle;
  static const TaskHandle TASK_HANDLE_INVALID = 0;


  typedef std::function<void(const std::string&)> TaskHandlerFunc;

  virtual ~IScheduler() {};

  virtual void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) = 0;
  virtual void unregisterMessageHandler(const std::string& clientId) = 0;

  virtual std::vector<std::string> getMyTasks(const std::string& clientId) const = 0;
  virtual std::string getMyTask(const std::string& clientId, const TaskHandle& hndl) const = 0;

  virtual TaskHandle scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp) = 0;
  virtual TaskHandle scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now()) = 0;

  virtual void removeAllMyTasks(const std::string& clientId) = 0;
  virtual void removeTask(const std::string& clientId, TaskHandle hndl) = 0;
  virtual void removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls) = 0;

  virtual void start() = 0;
  virtual void stop() = 0;
};
