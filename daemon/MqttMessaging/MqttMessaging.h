#pragma once

#include "IMessaging.h"
#include <string>

class IDaemon;
class Impl;

typedef std::basic_string<unsigned char> ustring;

class MqttMessaging : public IMessaging
{
public:
  MqttMessaging();
  virtual ~MqttMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

private:
  Impl* m_impl;
};

class MqttChannelException : public std::exception {
public:
  MqttChannelException(const std::string& cause)
    :m_cause(cause)
  {}

  //TODO ?
#ifndef WIN32
  virtual const char* what() const noexcept(true)
#else
  virtual const char* what() const
#endif
  {
    return m_cause.c_str();
  }

  virtual ~MqttChannelException()
  {}

protected:
  std::string m_cause;
};
