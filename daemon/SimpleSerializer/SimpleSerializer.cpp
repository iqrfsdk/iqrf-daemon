#include "SimpleSerializer.h"
#include "IqrfLogging.h"
#include <vector>
#include <iterator>
#include <array>

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
      }
      else {
        THROW_EX(std::logic_error, "Parse error: " << NAME_PAR(token, timeoutStr));
      }
    }
    else if (std::string::npos != (found = it->find("CLID"))) {
      std::string clientIdStr = *it;
      it = vstrings.erase(it);
      found = clientIdStr.find_first_of('=');
      if (std::string::npos != found && found < clientIdStr.size() - 1) {
        dpaTask.setClid(clientIdStr.substr(++found, clientIdStr.size() - 1));
      }
      else {
        THROW_EX(std::logic_error, "Parse error: " << NAME_PAR(token, clientIdStr));
      }
    }
    else
      it++;
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
  }
  else {
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
DpaTaskSimpleSerializerFactory::DpaTaskSimpleSerializerFactory()
{
  registerClass<PrfThermometerSimple>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGSimple>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRSimple>(PrfLedR::PRF_NAME);
  registerClass<PrfRawSimple>(PrfRawSimple::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskSimpleSerializerFactory::parseRequest(const std::string& request)
{
  std::unique_ptr<DpaTask> obj;
  try {
    std::istringstream istr(request);
    std::string perif;
    istr >> perif;
    obj = createObject(perif, istr);
    m_lastError = "OK";
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return std::move(obj);
}

std::string DpaTaskSimpleSerializerFactory::getLastError() const
{
  return m_lastError;
}
