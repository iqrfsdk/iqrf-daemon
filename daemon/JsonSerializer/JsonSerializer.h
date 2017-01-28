#pragma once

#include "ISerializer.h"
#include "ObjectFactory.h"
#include "PrfRaw.h"
#include "PrfFrc.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <memory>
#include <string>

class PrfCommonJson
{
public:
  PrfCommonJson() = delete;
  PrfCommonJson(DpaTask& dpaTask);
  void parseRequestJson(rapidjson::Value& val);
  void encodeResponseJson(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
private:
  DpaTask& m_dpaTask;
  bool m_explicitTimeout = false;
};

class PrfRawJson : public PrfRaw
{
public:
  explicit PrfRawJson(rapidjson::Value& val);
  virtual ~PrfRawJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common = *this;
  bool m_dotNotation = false;
};

class PrfThermometerJson : public PrfThermometer
{
public:
  explicit PrfThermometerJson(rapidjson::Value& val);
  virtual ~PrfThermometerJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common = *this;
};

class PrfFrcJson : public PrfFrc
{
public:
  explicit PrfFrcJson(rapidjson::Value& val);
  virtual ~PrfFrcJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common = *this;
  bool m_predefinedFrcCommand = false;
};

template <typename L>
class PrfLedJson : public L
{
public:
  explicit PrfLedJson(rapidjson::Value& val) {
    m_common.parseRequestJson(val);
  }

  virtual ~PrfLedJson() {}

  std::string encodeResponse(const std::string& errStr) const override
  {
    rapidjson::Document doc;
    doc.SetObject();

    m_common.encodeResponseJson(doc, doc.GetAllocator());

    rapidjson::Value v;
    v = L::getLedState();
    doc.AddMember("LedState", v, doc.GetAllocator());

    v.SetString(errStr.c_str(), doc.GetAllocator());
    doc.AddMember("Status", v, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
  }

private:
  PrfCommonJson m_common = *this;
};

typedef PrfLedJson<PrfLedG> PrfLedGJson;
typedef PrfLedJson<PrfLedR> PrfLedRJson;

class DpaTaskJsonSerializerFactory : public ObjectFactory<DpaTask, rapidjson::Value>, public ISerializer
{
public:
  virtual ~DpaTaskJsonSerializerFactory() {}
  DpaTaskJsonSerializerFactory();

  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string getLastError() const override;
private:
  std::string m_lastError;
};
