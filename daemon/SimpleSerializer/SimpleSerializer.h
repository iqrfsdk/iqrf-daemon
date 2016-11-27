#pragma once

#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include <memory>
#include <string>

void parseRequestCommon(std::istream& istr, DpaTask& dpaTask);
void encodeResponseCommon(std::ostream& ostr, DpaTask & dt);

class PrfThermometerSimple : public PrfThermometer
{
public:
  virtual ~PrfThermometerSimple();
  void parseRequestMessage(std::istream& istr) override;
  void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override;
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

class DpaTaskSimpleSerializerFactory : public ObjectFactory<std::string, DpaTask>
{
public:
  DpaTaskSimpleSerializerFactory();
  std::unique_ptr<DpaTask> parse(const std::string& request);
};
