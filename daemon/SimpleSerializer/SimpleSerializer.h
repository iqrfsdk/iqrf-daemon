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
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include <vector>
#include <string>

std::vector<std::string> parseTokens(DpaTask& dpaTask, std::istream& istr);
void parseRequestSimple(DpaTask& dpaTask, std::vector<std::string>& tokens);

void encodeResponseSimple(const DpaTask & dt, std::ostream& ostr);
void encodeTokens(const DpaTask& dpaTask, const std::string& errStr, std::ostream& ostr);

class PrfRawSimple : public PrfRaw
{
public:
  explicit PrfRawSimple(std::istream& istr);
  virtual ~PrfRawSimple() {};
  std::string encodeResponse(const std::string& errStr) const  override;
};

class PrfThermometerSimple : public PrfThermometer
{
public:
  explicit PrfThermometerSimple(std::istream& istr);
  virtual ~PrfThermometerSimple() {};
  std::string encodeResponse(const std::string& errStr) const  override;
};

template <typename L>
class PrfLedSimple : public L
{
public:
  explicit PrfLedSimple(std::istream& istr) {
    std::vector<std::string> v = parseTokens(*this, istr);
    parseRequestSimple(*this, v);
  }

  virtual ~PrfLedSimple() {}

  std::string encodeResponse(const std::string& errStr) const override
  {
    std::ostringstream ostr;
    encodeResponseSimple(*this, ostr);
    ostr << " " << L::getLedState();

    encodeTokens(*this, errStr, ostr);

    return ostr.str();
  }
};

typedef PrfLedSimple<PrfLedG> PrfLedGSimple;
typedef PrfLedSimple<PrfLedR> PrfLedRSimple;

class SimpleSerializer : public ISerializer
{
public:
  SimpleSerializer();
  SimpleSerializer(const std::string& name);
  virtual ~SimpleSerializer() {}

  //component
  const std::string& getName() const override { return m_name; }

  //interface
  const std::string& parseCategory(const std::string& request) override;
  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string parseConfig(const std::string& request) override;
  std::string getLastError() const override;
private:
  ObjectFactory<DpaTask, std::istream> m_dpaParser;

  void init();
  std::string m_lastError;
  std::string m_name;
};
