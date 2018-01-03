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
#include "DpaRaw.h"
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

/// \class PrfCommonJson
/// \brief Implements common features of JsonDpaMessage
/// \details
/// Common functions as parsing and encoding common items of JSON coded DPA messages
class PrfCommonJson
{
protected:

  PrfCommonJson();
  PrfCommonJson(const PrfCommonJson& o);

  /// \brief Parse common items
  /// \param [in] val JSON structure to be parsed
  /// \param [out] dpaTask reference to be set according parsed data
  /// \details
  /// Gets JSON encoded DPA request, parse it and set DpaTask object accordingly
  void parseRequestJson(const rapidjson::Value& val, DpaTask& dpaTask);

  /// \brief Encode begining members of JSON
  /// \param [in] dpaTask reference to be encoded
  /// \details
  /// Gets DPA request common members to be stored at the very beginning of JSON message
  void addResponseJsonPrio1Params(const DpaTask& dpaTask);

  /// \brief Encode middle members of JSON
  /// \param [in] dpaTask reference to be encoded
  /// \details
  /// Gets DPA request common members to be stored in the middle of JSON message
  void addResponseJsonPrio2Params(const DpaTask& dpaTask);

  /// \brief Encode final members of JSON and return it
  /// \param [in] dpaTask reference to be encoded
  /// \return complete JSON encoded message
  /// \details
  /// Gets DPA request common parameters to be stored at the end of JSON message and return complete JSON
  std::string encodeResponseJsonFinal(const DpaTask& dpaTask);

public:

  /// \brief Parse binary data encoded hexa
  /// \param [out] to buffer for result binary data
  /// \param [in] from hexadecimal string
  /// \param [in] maxlen maximal length of binary data
  /// \return length of result
  /// \details
  /// Gets hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation) and parses to binary data
  int parseBinary(uint8_t* to, const std::string& from, int maxlen);

  /// \brief Parse templated ordinary type T encoded hexa
  /// \param [out] to buffer for result binary data
  /// \param [in] from hexadecimal string
  /// \return length of result
  /// \details
  /// Gets hexadecimal string in form e.g: "00a5b1" and inerpret it as templated ordinary type
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

  /// \brief Encode uint_8 to hexa string
  /// \param [out] to encoded string
  /// \param [in] from value to be encoded
  void encodeHexaNum(std::string& to, uint8_t from);

  /// \brief Encode uint_16 to hexa string
  /// \param [out] to encoded string
  /// \param [in] from value to be encoded
  void encodeHexaNum(std::string& to, uint16_t from);

  /// \brief Encode binary data to hexa string
  /// \param [out] to result string
  /// \param [in] from data to be encoded
  /// \param [in] len length of dat to be encoded
  /// \details
  /// Encode binary data to hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation)
  /// Used separation is controlled by member m_dotNotation and it is hardcoded as dot separation it this version
  void encodeBinary(std::string& to, const uint8_t* from, int len);
  
  /// \brief Encode timestamp
  /// \param [out] to result string
  /// \param [in] from timestamp to be encoded
  void encodeTimestamp(std::string& to, std::chrono::time_point<std::chrono::system_clock> from);

  /// various flags to store presence of members of DPA request to be used in DPA response
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
  bool m_has_rcode = false;
  bool m_has_rdata = false;
  bool m_has_dpaval = false;

  /// various flags to store members of DPA request to be used in DPA response
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
  std::string m_rcodeJ;
  std::string m_rdataJ;
  std::string m_dpavalJ;

  rapidjson::Document m_doc;

  bool m_dotNotation = true;
};

/// \class PrfRawJson
/// \brief Parse/encode JSON message holding DpaRaw
/// \details
/// Class to be passed to parser as creator of DpaRaw object from incoming JSON.
/// See https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#raw for details
class PrfRawJson : public DpaRaw, public PrfCommonJson
{
public:
  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfRawJson(const rapidjson::Value& val);

  /// \brief parametric constructor
  /// \param [in] dpaMessage DPA async. message
  /// \details
  /// This is used to create PrfRaw from asynchronous message
  explicit PrfRawJson(const DpaMessage& dpaMessage);
  virtual ~PrfRawJson() {}
  
  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
  
  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh - asynchronous
  /// \return encoded message
  std::string encodeAsyncRequest(const std::string& errStr);
private:
};

/// \class PrfRawJson
/// \brief Parse/encode JSON message holding DpaRaw
/// \details
/// Class to be passed to parser as creator of DpaRaw object from incoming JSON. It parses/encodes DPA header.
/// See https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#raw-hdp for details
class PrfRawHdpJson : public DpaRaw, public PrfCommonJson
{
public:
  /// name to be registered in parser
  /// expected in JSON as: "type": "raw-hdp"
  static const std::string PRF_NAME;

  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfRawHdpJson(const rapidjson::Value& val);
  virtual ~PrfRawHdpJson() {}

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
private:
  /// parsed DPA header items
  std::string m_pnum;
  std::string m_pcmd;
  std::string m_data;

};

/// \class PrfThermometerJson
/// \brief Parse/encode JSON message holding PrfThermometer
/// \details
/// Class to be passed to parser as creator of PrfThermometer object from incoming JSON.
/// It will be probably reimplemented.
/// See: https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#predefined-embedded-perifery-types
class PrfThermometerJson : public PrfThermometer, public PrfCommonJson
{
public:
  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfThermometerJson(const rapidjson::Value& val);
  virtual ~PrfThermometerJson() {}

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
};

/// \class PrfFrcJson
/// \brief Parse/encode JSON message holding PrfFrcJson
/// \details
/// Class to be passed to parser as creator of PrfFrcJson object from incoming JSON.
/// It will be probably reimplemented.
/// See: https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#predefined-embedded-perifery-types
class PrfFrcJson : public PrfFrc, public PrfCommonJson
{
public:
  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfFrcJson(const rapidjson::Value& val);
  virtual ~PrfFrcJson() {}

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
private:
  bool m_predefinedFrcCommand = false;
  std::string m_userData;
};

/// \class PrfIoJson
/// \brief Parse/encode JSON message holding PrfIoJson
/// \details
/// Class to be passed to parser as creator of PrfIoJson object from incoming JSON.
/// It will be probably reimplemented.
/// See: https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#predefined-embedded-perifery-types
class PrfIoJson : public PrfIo, public PrfCommonJson
{
public:
  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfIoJson(const rapidjson::Value& val);
  virtual ~PrfIoJson() {}

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
private:
  Port m_port;
  uint8_t m_bit;
  bool m_val;
};

/// \class PrfOsJson
/// \brief Parse/encode JSON message holding PrfOsJson
/// \details
/// Class to be passed to parser as creator of PrfOsJson object from incoming JSON.
/// It will be probably reimplemented.
/// See: https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#predefined-embedded-perifery-types
class PrfOsJson : public PrfOs, public PrfCommonJson
{
public:
  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfOsJson(const rapidjson::Value& val);
  virtual ~PrfOsJson() {}

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
};

/// \class PrfLedJson
/// \brief Parse/encode JSON message holding PrfLed
/// \details
/// Class to be passed to parser as creator of PrfLed object from incoming JSON.
/// It will be probably reimplemented.
/// See: https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#predefined-embedded-perifery-types
template <typename L>
class PrfLedJson : public L, public PrfCommonJson
{
public:
  /// \brief parametric constructor
  /// \param [in] val JSON to be parsed
  explicit PrfLedJson(const rapidjson::Value& val) {
    parseRequestJson(val, *this);
  }

  virtual ~PrfLedJson() {}


  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
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

/// Type for embedded Green LED
typedef PrfLedJson<PrfLedG> PrfLedGJson;

/// Type for embedded Red LED
typedef PrfLedJson<PrfLedR> PrfLedRJson;

/// \class JsonSerializer
/// \brief Object factory to create DpaTask objects from incoming messages
/// \details
/// Uses inherited ObjectFactory features to create DpaTask object from incoming JSON messages.
class JsonSerializer : public ObjectFactory<DpaTask, rapidjson::Value>, public ISerializer
{
public:
  JsonSerializer();

  /// parametric constructor
  /// \param [in] name instance name
  JsonSerializer(const std::string& name);
  virtual ~JsonSerializer() {}

  /// \brief returns instance name
  const std::string& getName() const override { return m_name; }

  /// ISerializer overriden methods
  std::string parseCategory(const std::string& request) override;
  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string parseConfig(const std::string& request) override;
  std::string encodeConfig(const std::string& request, const std::string& response) override;
  std::string getLastError() const override;
  std::string encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const override;

private:
  void init();
  std::string m_lastError;
  std::string m_name;
};
