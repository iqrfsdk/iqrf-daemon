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
protected:

  PrfCommonJson();
  void parseRequestJson(rapidjson::Value& val);
  std::string encodeResponseJson(const DpaTask& dpaTask);

public:
  void encodeBinary(std::string& to, unsigned char* from, int len);

  bool m_has_ctype = false;
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

  std::string m_ctype;
  std::string m_type;
  int m_nadr = -1;
  std::string m_hwpid = "0xffff";
  int m_timeoutJ = 0;
  std::string m_msgid;
  std::string m_requestJ;
  std::string m_request_ts;
  std::string m_responseJ;
  std::string m_response_ts;
  std::string m_confirmationJ;
  std::string m_confirmation_ts;
  std::string m_cmdJ;
  std::string m_statusJ;

  mutable rapidjson::Document m_doc;

};

class PrfRawJson : public PrfRaw, public PrfCommonJson
{
public:
  explicit PrfRawJson(rapidjson::Value& val);
  virtual ~PrfRawJson() {}
  std::string encodeResponse(const std::string& errStr) override;
private:
  bool m_dotNotation = false;
};

class PrfThermometerJson : public PrfThermometer, public PrfCommonJson
{
public:
  explicit PrfThermometerJson(rapidjson::Value& val);
  virtual ~PrfThermometerJson() {}
  std::string encodeResponse(const std::string& errStr) override;
};

class PrfFrcJson : public PrfFrc, public PrfCommonJson
{
public:
  explicit PrfFrcJson(rapidjson::Value& val);
  virtual ~PrfFrcJson() {}
  std::string encodeResponse(const std::string& errStr) override;
private:
  bool m_predefinedFrcCommand = false;
};

class PrfIoJson : public PrfIo, public PrfCommonJson
{
public:
  explicit PrfIoJson(rapidjson::Value& val);
  virtual ~PrfIoJson() {}
  std::string encodeResponse(const std::string& errStr) override;
private:
  Port m_port;
  uint8_t m_bit;
  bool m_val;
};

class PrfOsJson : public PrfOs, public PrfCommonJson
{
public:
  explicit PrfOsJson(rapidjson::Value& val);
  virtual ~PrfOsJson() {}
  std::string encodeResponse(const std::string& errStr) override;
};

template <typename L>
class PrfLedJson : public L, public PrfCommonJson
{
public:
  explicit PrfLedJson(rapidjson::Value& val) {
    parseRequestJson(val);
    setAddress((uint16_t)m_nadr);
    parseCommand(m_cmdJ);
    if (m_timeoutJ >= 0) {
      setTimeout(m_timeoutJ);
    }
  }

  virtual ~PrfLedJson() {}

  std::string encodeResponse(const std::string& errStr) override
  {
    rapidjson::Value v;

    v = L::getLedState();
    m_doc.AddMember("LedState", v, m_doc.GetAllocator());

    m_statusJ = errStr;
    return encodeResponseJson(*this);
  }

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
