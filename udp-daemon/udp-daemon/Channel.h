#pragma once

#include <string>
#include <functional>

class Channel
{
public:
  typedef std::function<void(const std::basic_string<unsigned char>&)> ReceiveFromFunc;
  virtual ~Channel() {};
  virtual void sendTo(const std::basic_string<unsigned char>& message) = 0;
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) = 0;
};
