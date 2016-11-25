#pragma once

#include "rapidjson/rapidjson.h"
#include <string>

class JsonCoding
{
public:
  JsonCoding();
  virtual ~JsonCoding();

private:
  int handleScheduledRecord(const std::string& msg);
};
