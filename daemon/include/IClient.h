#pragma once

#include "JsonUtils.h"
#include <string>
#include <functional>

class IDaemon;
class ISerializer;
class IMessaging;

class IClient
{
public:
  //component
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void update(const rapidjson::Value& cfg) =0;
  virtual const std::string& getClientName() const = 0;

  //references
  virtual void setDaemon(IDaemon* daemon) = 0;
  virtual void setSerializer(ISerializer* serializer) = 0;
  virtual void setMessaging(IMessaging* messaging) = 0;

  //interface
  
  inline virtual ~IClient() {};
};
