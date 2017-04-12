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

#include "LaunchUtils.h"
#include "SimpleSerializer.h"
#include "IqrfLogging.h"
#include <vector>
#include <iterator>
#include <array>

INIT_COMPONENT(ISerializer, SimpleSerializer)

std::vector<std::string> parseTokens(DpaTask& dpaTask, std::istream& istr)
{
  std::istream_iterator<std::string> begin(istr);
  std::istream_iterator<std::string> end;
  std::vector<std::string> vstrings(begin, end);

  std::size_t found;
  auto it = vstrings.begin();
  while (it != vstrings.end()) {
    if (std::string::npos != (found = it->find("TIMEOUT"))) {
      std::string timeoutStr = *it;
      it = vstrings.erase(it);
      found = timeoutStr.find_first_of('=');
      if (std::string::npos != found && found < timeoutStr.size()-1) {
        dpaTask.setTimeout(std::stoi(timeoutStr.substr(++found, timeoutStr.size() - 1)));
      } else {
        THROW_EX(std::logic_error, "Parse error: " << NAME_PAR(token, timeoutStr));
      }
    }
    else if (std::string::npos != (found = it->find("CLID"))) {
      std::string clientIdStr = *it;
      it = vstrings.erase(it);
      found = clientIdStr.find_first_of('=');
      if (std::string::npos != found && found < clientIdStr.size() - 1) {
        dpaTask.setClid(clientIdStr.substr(++found, clientIdStr.size() - 1));
      } else {
        THROW_EX(std::logic_error, "Parse error: " << NAME_PAR(token, clientIdStr));
      }
    } else {
      it++;
    }
  }

  return vstrings;
}

void parseRequestSimple(DpaTask & dpaTask, std::vector<std::string>& tokens)
{
  int address = -1;
  std::string command;
  if (tokens.size() > 1) {
    address = std::stoi(tokens[0]);
    command = tokens[1];
    dpaTask.setAddress(address);
    dpaTask.parseCommand(command);
  } else {
    THROW_EX(std::logic_error, "Parse error: " << NAME_PAR(tokensNum, tokens.size()));
  }
}

void encodeResponseSimple(const DpaTask & dt, std::ostream& ostr)
{
  ostr << dt.getPrfName() << " " << dt.getAddress() << " " << dt.encodeCommand() << " ";
}

void encodeTokens(const DpaTask& dpaTask, const std::string& errStr, std::ostream& ostr)
{
  if (dpaTask.getTimeout() >= 0) {
    ostr << " " << "TIMEOUT=" << dpaTask.getTimeout();
  }
  if (!dpaTask.getClid().empty()) {
    ostr << " " << "CLID=" << dpaTask.getClid();
  }
  ostr << " " << errStr;
}

//////////////////////////////////////////

//00 00 06 03 ff ff  LedR 0 PULSE
//00 00 07 03 ff ff  LedG 0 PULSE

PrfRawSimple::PrfRawSimple(std::istream& istr)
  :PrfRaw()
{
  uint8_t bbyte;
  int val;
  int i = 0;

  std::vector<std::string> vstrings = parseTokens(*this, istr);

  int sz = (int)vstrings.size();
  while (i < MAX_DPA_BUFFER && i < sz) {
    val = std::stoi(vstrings[i], nullptr, 16);
    m_request.DpaPacket().Buffer[i] = (uint8_t)val;
    i++;
  }
  m_request.SetLength(i);
}

std::string PrfRawSimple::encodeResponse(const std::string& errStr) const
{
  std::ostringstream ostr;
  int len = m_response.Length();
  TRC_DBG(PAR(len));
  ostr << getPrfName() << " " <<
    iqrf::TracerHexString((unsigned char*)m_response.DpaPacket().Buffer, m_response.Length(), true);

  encodeTokens(*this, errStr, ostr);

  return ostr.str();
}

//////////////////////////////////////////
PrfThermometerSimple::PrfThermometerSimple(std::istream& istr)
{
  std::vector<std::string> v = parseTokens(*this, istr);
  parseRequestSimple(*this, v);
}

std::string PrfThermometerSimple::encodeResponse(const std::string& errStr) const
{
  std::ostringstream ostr;
  encodeResponseSimple(*this, ostr);
  ostr << " " << getFloatTemperature();

  encodeTokens(*this, errStr, ostr);

  return ostr.str();
}

///////////////////////////////////////////
SimpleSerializer::SimpleSerializer()
  :m_name("Simple")
{
  init();
}

SimpleSerializer::SimpleSerializer(const std::string& name)
  :m_name(name)
{
  init();
}

void SimpleSerializer::init()
{
  m_dpaParser.registerClass<PrfThermometerSimple>(PrfThermometer::PRF_NAME);
  m_dpaParser.registerClass<PrfLedGSimple>(PrfLedG::PRF_NAME);
  m_dpaParser.registerClass<PrfLedRSimple>(PrfLedR::PRF_NAME);
  m_dpaParser.registerClass<PrfRawSimple>(PrfRawSimple::PRF_NAME);
}

const std::string& SimpleSerializer::parseCategory(const std::string& request)
{
  std::istringstream istr(request);
  std::string category;
  istr >> category;
  if (category == CAT_CONF_STR) {
    return CAT_CONF_STR;
  }
  else {
    return CAT_DPA_STR;
  }
}

std::unique_ptr<DpaTask> SimpleSerializer::parseRequest(const std::string& request)
{
  std::unique_ptr<DpaTask> obj;
  try {
    std::istringstream istr(request);
    std::string perif;
    istr >> perif;
    obj = m_dpaParser.createObject(perif, istr);
    m_lastError = "OK";
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return std::move(obj);
}

std::string SimpleSerializer::parseConfig(const std::string& request)
{
  std::istringstream istr(request);
  std::string category, command;
  istr >> category >> command;
  if (category == CAT_CONF_STR) {
    m_lastError = "OK";
    return command;
  } else {
    std::ostringstream ostr;
    ostr << "Unexpected: " << PAR(category);
    m_lastError = ostr.str();
    return "";
  }
}

std::string SimpleSerializer::getLastError() const
{
  return m_lastError;
}
