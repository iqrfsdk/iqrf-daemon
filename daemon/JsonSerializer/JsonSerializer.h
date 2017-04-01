/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "ISerializer.h"
#include "ObjectFactory.h"
#include "PrfRaw.h"
#include "PrfFrc.h"
#include "PrfThermometer.h"
#include "PrfIo.h"
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

class PrfIoJson : public PrfIo
{
public:
  explicit PrfIoJson(rapidjson::Value& val);
  virtual ~PrfIoJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common = *this;
  Port m_port;
  uint8_t m_bit;
  bool m_val;
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

class JsonSerializer : public ObjectFactory<DpaTask, rapidjson::Value>, public ISerializer
{
public:
  JsonSerializer();
  JsonSerializer(const std::string& name);
  virtual ~JsonSerializer() {}

  //component
  const std::string& getName() const override { return m_name; }

  //interface
  const std::string& parseCategory(const std::string& request) override;
  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string parseConfig(const std::string& request) override;
  std::string getLastError() const override;
private:
  void init();
  std::string m_lastError;
  std::string m_name;
};
