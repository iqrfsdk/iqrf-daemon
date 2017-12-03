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
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include <algorithm>
#include <vector>
#include <string>

/// auxiliar parse/encode functions
std::vector<std::string> parseTokens(DpaTask& dpaTask, std::istream& istr);
void parseRequestSimple(DpaTask& dpaTask, std::vector<std::string>& tokens);
void encodeResponseSimple(const DpaTask & dt, std::ostream& ostr);
void encodeTokens(const DpaTask& dpaTask, const std::string& errStr, std::ostream& ostr);

/// \class PrfRawSimple
/// \brief Parse/encode simple message holding raw DPA message
/// \details
/// Class to be passed to parser as creator of DpaRaw object from incoming data.
/// expected message e.g: "raw 01.00.06.03.ff.ff"
/// or: "raw 01.00.06.03.ff.ff  timeout 1000"
class PrfRawSimple : public DpaRaw
{
public:
  /// \brief parametric constructor
  /// \param [in] istr string stream to be parsed
  explicit PrfRawSimple(std::istream& istr);
  virtual ~PrfRawSimple() {};
  
  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
private:
  bool m_dotNotation = true;
};

/// \class PrfThermometerSimple
/// \brief Parse/encode simple message holding PrfThermometer
/// \details
/// Class to be passed to parser as creator of PrfThermometer object from incoming data.
class PrfThermometerSimple : public PrfThermometer
{
public:
  /// \brief parametric constructor
  /// \param [in] istr string stream to be parsed
  explicit PrfThermometerSimple(std::istream& istr);
  virtual ~PrfThermometerSimple() {};

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override;
};

/// \class PrfLedSimple
/// \brief Parse/encode simple message holding PrfLed
/// \details
/// Class to be passed to parser as creator of PrfLed object from incoming data.
template <typename L>
class PrfLedSimple : public L
{
public:
  /// \brief parametric constructor
  /// \param [in] istr string stream to be parsed
  explicit PrfLedSimple(std::istream& istr) {
    std::vector<std::string> v = parseTokens(*this, istr);
    parseRequestSimple(*this, v);
  }

  virtual ~PrfLedSimple() {}

  /// \brief DpaTask overriden method
  /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
  /// \return encoded message
  std::string encodeResponse(const std::string& errStr) override
  {
    std::ostringstream ostr;
    encodeResponseSimple(*this, ostr);
    ostr << " " << L::getLedState();

    encodeTokens(*this, errStr, ostr);

    return ostr.str();
  }
};

/// Type for embedded Green LED
typedef PrfLedSimple<PrfLedG> PrfLedGSimple;

/// Type for embedded Red LED
typedef PrfLedSimple<PrfLedR> PrfLedRSimple;

/// \class SimpleSerializer
/// \brief Object factory to create DpaTask objects from incoming messages
/// \details
/// Uses inherited ObjectFactory features to create DpaTask object from incoming simple messages.
class SimpleSerializer : public ISerializer
{
public:
  SimpleSerializer();
 
  /// parametric constructor
  /// \param [in] name instance name
  SimpleSerializer(const std::string& name);
  virtual ~SimpleSerializer() {}

  /// ISerializer overriden methods
  const std::string& getName() const override { return m_name; }
  std::string parseCategory(const std::string& request) override;
  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string parseConfig(const std::string& request) override;
  std::string encodeConfig(const std::string& request, const std::string& response) override;
  std::string getLastError() const override;
  std::string encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const override;

private:
  ObjectFactory<DpaTask, std::istream> m_dpaParser;

  void init();
  std::string m_lastError;
  std::string m_name;
};
