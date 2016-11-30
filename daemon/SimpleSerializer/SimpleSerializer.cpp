#include "SimpleSerializer.h"
#include "IqrfLogging.h"

void parseRequestSimple(std::istream& istr, DpaTask& dpaTask)
{
  int address = -1;
  std::string command;

  istr >> address >> command;

  dpaTask.setAddress(address);
  dpaTask.parseCommand(command);
}

void encodeResponseSimple(std::ostream& ostr, DpaTask & dt)
{
  ostr << dt.getPrfName() << " " << dt.getAddress() << " " << dt.encodeCommand() << " ";
}

PrfThermometerSimple::PrfThermometerSimple(std::istream& istr)
{
  parseRequestSimple(istr, *this);
}

PrfThermometerSimple::~PrfThermometerSimple()
{
}

void PrfThermometerSimple::encodeResponseMessage(std::ostream& ostr, const std::string& errStr)
{
  encodeResponseSimple(ostr, *this);
  ostr << " " << getFloatTemperature() << " " << errStr;
}

///////////////////////////////////////////
DpaTaskSimpleSerializerFactory::DpaTaskSimpleSerializerFactory()
{
  registerClass<PrfThermometerSimple>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGSimple>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRSimple>(PrfLedR::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskSimpleSerializerFactory::parse(const std::string& request)
{
  std::istringstream istr(request);
  std::string perif;
  istr >> perif;
  auto obj = createObject(perif, istr);
  return std::move(obj);
}
