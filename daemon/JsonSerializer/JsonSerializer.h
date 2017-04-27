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
#include "PrfOs.h"
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
  PrfCommonJson();
  //PrfCommonJson(DpaTask& dpaTask);
  void parseRequestJson(rapidjson::Value& val);
  void encodeResponseJson(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  /*
  void encode_nadr(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_hwpid(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_timeout(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_msgid(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_request(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_request_ts(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_response(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_response_ts(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_confirmation(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_confirmation_ts(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  void encode_cmd(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const;
  */

public:
  bool has_nadr() { return m_has_nadr; }
  bool has_hwpid() { return m_has_hwpid; }
  bool has_timeout() { return m_has_timeout; }
  bool has_msgid() { return m_has_msgid; }
  bool has_request() { return m_has_request; }
  bool has_request_ts() { return m_has_request_ts; }
  bool has_response() { return m_has_response; }
  bool has_response_ts() { return m_has_response_ts; }
  bool has_confirmation() { return m_has_confirmation; }
  bool has_confirmation_ts() { return m_has_confirmation_ts; }
  bool has_cmd() { return m_has_cmd; }

  bool m_has_type = false;
  bool m_has_nadr = false;
  bool m_has_hwpid = false;
  bool m_has_timeout = false;
  bool m_has_msgid = false;
  bool m_has_request = false;
  bool m_has_request_ts = false;
  bool m_has_response = false;
  bool m_has_response_ts = false;
  bool m_has_confirmation = false;
  bool m_has_confirmation_ts = false;
  bool m_has_cmd = false;

  std::string m_type;
  int m_nadr = -1;
  std::string m_hwpid = "0xffff";
  int m_timeout = 0;
  std::string m_msgid;
  std::string m_request;
  std::string m_request_ts;
  std::string m_response;
  std::string m_response_ts;
  std::string m_confirmation;
  std::string m_confirmation_ts;
  std::string m_cmd;

  //DpaTask& m_dpaTask;
};

class PrfRawJson : public PrfRaw
{
public:
  explicit PrfRawJson(rapidjson::Value& val);
  virtual ~PrfRawJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common;
  bool m_dotNotation = false;
};

class PrfThermometerJson : public PrfThermometer
{
public:
  explicit PrfThermometerJson(rapidjson::Value& val);
  virtual ~PrfThermometerJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common;
};

class PrfFrcJson : public PrfFrc
{
public:
  explicit PrfFrcJson(rapidjson::Value& val);
  virtual ~PrfFrcJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common;
  bool m_predefinedFrcCommand = false;
};

class PrfIoJson : public PrfIo
{
public:
  explicit PrfIoJson(rapidjson::Value& val);
  virtual ~PrfIoJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common;
  Port m_port;
  uint8_t m_bit;
  bool m_val;
};

class PrfOsJson : public PrfOs
{
public:
  explicit PrfOsJson(rapidjson::Value& val);
  virtual ~PrfOsJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common;
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
  PrfCommonJson m_common;
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
