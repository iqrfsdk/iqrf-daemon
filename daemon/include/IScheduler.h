#pragma once

#include <string>
#include <functional>

class IScheduler
{
public:
  typedef std::basic_string<unsigned char> ustring;
  typedef std::function<void(const ustring&)> TaskHandlerFunc;

  virtual ~IScheduler() {};
  virtual void makeCommand(const std::string& clientId, const std::string& command) = 0;
  virtual void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) = 0;
  virtual void unregisterMessageHandler(const std::string& clientId) = 0;

  virtual void start() = 0;
  virtual void stop() = 0;
};
