#pragma once

#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <memory>
#include <string>

//void parseRequestCommonJson(std::istream& istr, DpaTask& dpaTask);
//void encodeResponseCommonJson(std::ostream& ostr, DpaTask & dt);

class JsonSerialized
{
public:
  virtual ~JsonSerialized() {};

  std::unique_ptr<DpaTask> moveDpaTask() {
    return std::move(m_dpaTask);
  }

  virtual void parseRequestJson(const rapidjson::Document& doc);
  virtual void encodeResponseJson(rapidjson::Document& doc, const std::string& errStr);

protected:
  std::unique_ptr<DpaTask> m_dpaTask;
};

class PrfThermometerJson: public JsonSerialized
{
public:
  PrfThermometerJson();
  virtual ~PrfThermometerJson();
  void parseRequestJson(const rapidjson::Document& doc) override;
  void encodeResponseJson(rapidjson::Document& doc, const std::string& errStr) override;
};

#if 0
template <typename L>
class PrfLedJson : public L
{
public:
  virtual ~PrfLedJson() {}

  void parseRequestJson(const rapidjson::Document& doc) override
  {
    parseRequestJson(istr, *this);
    m_valid = true;
  }

  void encodeResponseJson(rapidjson::Document& doc, const std::string& errStr) override;
  {
    encodeResponseJson(ostr, *this);
    ostr << " " << getLedState();
  }

};

typedef PrfLedJson<PrfLedG> PrfLedGJson;
typedef PrfLedJson<PrfLedR> PrfLedRJson;
#endif

class DpaTaskJsonSerializerFactory : public ObjectFactory<std::string, JsonSerialized>
{
public:
  DpaTaskJsonSerializerFactory();
  std::unique_ptr<DpaTask> parse(const std::string& request);
};
