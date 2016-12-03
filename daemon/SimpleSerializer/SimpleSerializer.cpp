#include "SimpleSerializer.h"
#include "IqrfLogging.h"
#include <array>

void parseRequestSimple(std::istream& istr, DpaTask& dpaTask)
{
  int address = -1;
  std::string command;

  istr >> address >> command;

  dpaTask.setAddress(address);
  dpaTask.parseCommand(command);
}

void encodeResponseSimple(const DpaTask & dt, std::ostream& ostr)
{
  ostr << dt.getPrfName() << " " << dt.getAddress() << " " << dt.encodeCommand() << " ";
}


//////////////////////////////////////////

//00 00 06 03 ff ff  LedR 0 PULSE
//00 00 07 03 ff ff  LedG 0 PULSE

PrfRawSimple::PrfRawSimple(std::istream& istr)
  :DpaRawTask()
{
  uint8_t bbyte;
  int val;
  int i = 0;

  while (i < MAX_DPA_BUFFER) {
    if (istr >> std::hex >> val) {
      m_request.DpaPacket().Buffer[i] = (uint8_t)val;
      i++;
    }
    else
      break;
  }
  m_request.SetLength(i);
}

std::string PrfRawSimple::encodeResponse(const std::string& errStr) const
{
  std::ostringstream ostr;
  int len = m_response.Length();
  TRC_DBG(PAR(len));
  ostr << iqrf::TracerHexString((unsigned char*)m_response.DpaPacket().Buffer, m_response.Length(), true);
  return ostr.str();
}

//////////////////////////////////////////
PrfThermometerSimple::PrfThermometerSimple(std::istream& istr)
{
  parseRequestSimple(istr, *this);
}

std::string PrfThermometerSimple::encodeResponse(const std::string& errStr) const
{
  std::ostringstream ostr;
  encodeResponseSimple(*this, ostr);
  ostr << " " << getFloatTemperature() << " " << errStr;
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
  std::istringstream istr(request);
  std::string perif;
  istr >> perif;
  auto obj = createObject(perif, istr);
  return std::move(obj);
}

std::string DpaTaskSimpleSerializerFactory::getLastError() const
{
  return "OK";
}
