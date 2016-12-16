#pragma once

#include <string>
#include <functional>
#include <vector>

class IScheduler
{
public:
  //typedef std::basic_string<unsigned char> ustring;
  typedef std::function<void(const std::string&)> TaskHandlerFunc;

  virtual ~IScheduler() {};
  
  virtual void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) = 0;
  virtual void unregisterMessageHandler(const std::string& clientId) = 0;

  virtual std::vector<std::string> getMyTasks(const std::string& clientId) = 0;
  virtual void removeAllMyTasks(const std::string& clientId) = 0;
  
  //virtual std::vector<std::string> removeMyTask(const std::string& clientId, const std::string& task) = 0;

  virtual void start() = 0;
  virtual void stop() = 0;
};
