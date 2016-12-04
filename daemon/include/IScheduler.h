#pragma once

#include <string>
#include <functional>

class IScheduler
{
public:
  typedef std::basic_string<unsigned char> ustring;
  typedef std::function<void(const ustring&)> MessageHandlerFunc;

  virtual ~IScheduler() {};
  virtual void makeCommand(const std::string& clientId, const std::string& command) = 0;
  virtual void registerResponseHandler(const std::string& clientId, MessageHandlerFunc fun) = 0;
  virtual void unregisterResponseHandler(const std::string& clientId) = 0;
};
