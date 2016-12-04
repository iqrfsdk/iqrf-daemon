#pragma once

#include <string>
#include <functional>

class IDaemon;

class IClient
{
public:
  virtual ~IClient() {};
  virtual void setDaemon(IDaemon* daemon) = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual const std::string& getClientName() = 0;
};
