#pragma once

#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <memory>
#include <string>

void parseRequestJson(DpaTask& dpaTask, rapidjson::Value& val);
void encodeResponseJson(DpaTask& dpaTask, rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc);

class PrfThermometerJson : public PrfThermometer
{
public:
  explicit PrfThermometerJson(rapidjson::Value& val);
  virtual ~PrfThermometerJson() {}
  void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override;
};

template <typename L>
class PrfLedJson : public L
{
public:
  explicit PrfLedJson(rapidjson::Value& val) {
    parseRequestJson(*this, val);
  }

  virtual ~PrfLedJson() {}

  void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override
  {
    Document doc;
    doc.SetObject();

    encodeResponseJson(*this, doc, doc.GetAllocator());

    rapidjson::Value v;
    v = getLedState();
    doc.AddMember("LedState", v, doc.GetAllocator());

    v.SetString(errStr.c_str(), doc.GetAllocator());
    doc.AddMember("Status", v, doc.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    ostr << buffer.GetString();
  }

};

typedef PrfLedJson<PrfLedG> PrfLedGJson;
typedef PrfLedJson<PrfLedR> PrfLedRJson;

class DpaTaskJsonSerializerFactory : public ObjectFactory<DpaTask, rapidjson::Value>
{
public:
  DpaTaskJsonSerializerFactory();
  std::unique_ptr<DpaTask> parse(const std::string& request);
  std::string serializeError(const std::string& err);
};
