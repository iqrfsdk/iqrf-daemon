#pragma once

#include <string>
#include <functional>

class IChannel
{
public:
  typedef std::function<int(const std::basic_string<unsigned char>&)> ReceiveFromFunc;
  virtual ~IChannel() {};
  virtual void sendTo(const std::basic_string<unsigned char>& message) = 0;
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) = 0;
};
