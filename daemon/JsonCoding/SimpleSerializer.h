#pragma once

#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include <memory>
#include <string>

inline void parseRequestCommon(std::istream& istr, DpaTask& dpaTask)
{
  int address = -1;
  std::string command;

  istr >> address >> command;

  dpaTask.setAddress(address);
  dpaTask.parseCommand(command);
}

inline void encodeResponseCommon(std::ostream& ostr, DpaTask & dt)
{
  ostr << dt.getPrfName() << " " << dt.getAddress() << " " << dt.encodeCommand() << " ";
}

class PrfThermometerSimple : public PrfThermometer
{
public:
  virtual ~PrfThermometerSimple() {}

  void parseRequestMessage(std::istream& istr) override
  {
    parseRequestCommon(istr, *this);
    m_valid = true;
  }

  void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override
  {
    encodeResponseCommon(ostr, *this);
    ostr << " " << getFloatTemperature();
  }
};

template <typename L>
class PrfLedSimple : public L
{
public:
  virtual ~PrfLedSimple() {}

  void parseRequestMessage(std::istream& istr) override
  {
    parseRequestCommon(istr, *this);
    m_valid = true;
  }

  void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override
  {
    encodeResponseCommon(ostr, *this);
    ostr << " " << getLedState();
  }
};

typedef PrfLedSimple<PrfLedG> PrfLedGSimple;
typedef PrfLedSimple<PrfLedR> PrfLedRSimple;

///////////////////////////////////
class DpaTaskSimpleSerializerFactory : public ObjectFactory<std::string, DpaTask>
{
public:
  DpaTaskSimpleSerializerFactory()
  {
    registerClass<PrfThermometerSimple>(PrfThermometer::PRF_NAME);
    registerClass<PrfLedGSimple>(PrfLedG::PRF_NAME);
    registerClass<PrfLedRSimple>(PrfLedR::PRF_NAME);
  }

  std::unique_ptr<DpaTask> parse(const std::string& request)
  {
    std::istringstream istr(request);
    std::string perif;
    istr >> perif;
    auto obj = createObject(perif);
    obj->parseRequestMessage(istr);
    return std::move(obj);
  }
};
