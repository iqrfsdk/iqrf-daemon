#pragma once

#include <string>
#include <functional>

class IScheduler
{
public:
  typedef std::function<void(const std::string&)> ResponseHandlerFunc;

  virtual ~IScheduler() {};
  virtual void makeCommand(const std::string& clientId, const std::string& command) = 0;
  virtual void registerResponseHandler(const std::string& clientId, ResponseHandlerFunc fun) = 0;
  virtual void unregisterResponseHandler(const std::string& clientId) = 0;
};
