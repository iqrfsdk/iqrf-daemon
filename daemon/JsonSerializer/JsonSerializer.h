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
#include <algorithm>
#include <memory>
#include <string>

class PrfCommonJson
{
protected:

  PrfCommonJson();
  void parseRequestJson(rapidjson::Value& val, DpaTask& dpaTask);
  void addResponseJsonPrio1Params(const DpaTask& dpaTask);
  void addResponseJsonPrio2Params(const DpaTask& dpaTask);
  std::string encodeResponseJsonFinal(const DpaTask& dpaTask);

public:
  int parseBinary(uint8_t* to, const std::string& from, int maxlen);
  
  template<typename T>
  void parseHexaNum(T& to, const std::string& from)
  {
    int val = 0;
    std::istringstream istr(from);
    if (istr >> std::hex >> val) {
      to = (T)val;
    }
    else {
      THROW_EX(std::logic_error, "Unexpected format: " << PAR(from));
    }
  }

  void encodeHexaNum(std::string& to, uint8_t from);
  void encodeHexaNum(std::string& to, uint16_t from);

  void encodeBinary(std::string& to, const uint8_t* from, int len);
  void encodeTimestamp(std::string& to, std::chrono::time_point<std::chrono::system_clock> from);

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
  std::string m_nadr = "0";
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

  bool m_dotNotation = false;
};

class PrfRawJson : public PrfRaw, public PrfCommonJson
{
public:
  explicit PrfRawJson(rapidjson::Value& val);
  virtual ~PrfRawJson() {}
  std::string encodeResponse(const std::string& errStr) override;
private:
};

class PrfRawHdpJson : public PrfRaw, public PrfCommonJson
{
public:
  static const std::string PRF_NAME;

  explicit PrfRawHdpJson(rapidjson::Value& val);
  virtual ~PrfRawHdpJson() {}
  std::string encodeResponse(const std::string& errStr) override;
private:
  std::string m_pnum;
  std::string m_pcmd;
  std::string m_data;

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
  std::string m_userData;
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
    parseRequestJson(val, *this);
  }

  virtual ~PrfLedJson() {}

  std::string encodeResponse(const std::string& errStr) override
  {
    rapidjson::Value v;

    addResponseJsonPrio1Params(*this);
    addResponseJsonPrio2Params(*this);

    int ls = L::getLedState();
    if (ls >= 0) {
      v.SetString((ls ? "on" : "off"), m_doc.GetAllocator());
      m_doc.AddMember("led_state", v, m_doc.GetAllocator());
    }

    m_statusJ = errStr;
    return encodeResponseJsonFinal(*this);
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
  std::string parseCategory(const std::string& request) override;
  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string parseConfig(const std::string& request) override;
  std::string getLastError() const override;
private:
  void init();
  std::string m_lastError;
  std::string m_name;
};
