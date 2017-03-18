#pragma once

#include "ObjectFactory.h"
#include "DpaTask.h"
#include <memory>
#include <string>

class ISerializer
{
public:
  //component
  virtual const std::string& getName() const = 0;

  //interface
  virtual std::unique_ptr<DpaTask> parseRequest(const std::string& request) = 0;
  virtual std::string getLastError() const = 0;

  virtual ~ISerializer() {}
};
