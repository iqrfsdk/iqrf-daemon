#pragma once

#include <string>
#include <functional>

class IDaemon;

class IMessaging
{
public:
  typedef std::basic_string<unsigned char> ustring;
  typedef std::function<void(const ustring&)> MessageHandlerFunc;

  virtual ~IMessaging() {};
  virtual void setDaemon(IDaemon* daemon) = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void registerMessageHandler(MessageHandlerFunc hndl) = 0;
  virtual void unregisterMessageHandler() = 0;
  virtual void sendMessage(const ustring& msg) = 0;
};
