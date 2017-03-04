#pragma once

#include "JsonUtils.h"
#include <string>
#include <functional>

class IDaemon;

class IMessaging
{
public:
  //component
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void update(const rapidjson::Value& cfg) = 0;
  virtual const std::string& getName() const = 0;

  //references
  //virtual void setDaemon(IDaemon* daemon) = 0;

  //interface
  typedef std::basic_string<unsigned char> ustring;
  typedef std::function<void(const ustring&)> MessageHandlerFunc;
  virtual void registerMessageHandler(MessageHandlerFunc hndl) = 0;
  virtual void unregisterMessageHandler() = 0;
  virtual void sendMessage(const ustring& msg) = 0;

  inline virtual ~IMessaging() {};
};
