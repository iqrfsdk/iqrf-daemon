#pragma once

#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include <memory>
#include <string>

void parseRequestSimple(std::istream& istr, DpaTask& dpaTask);
void encodeResponseSimple(std::ostream& ostr, DpaTask & dt);

class PrfThermometerSimple : PrfThermometer
{
public:
  explicit PrfThermometerSimple(std::istream& istr);
  virtual ~PrfThermometerSimple();
  virtual void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override;
};

template <typename L>
class PrfLedSimple : public L
{
public:
  explicit PrfLedSimple(std::istream& istr) {
    parseRequestSimple(istr, *this);
  }

  virtual ~PrfLedSimple() {}

  void encodeResponseMessage(std::ostream& ostr, const std::string& errStr) override
  {
    encodeResponseSimple(ostr, *this);
    ostr << " " << getLedState() << " " << errStr;
  }
};

typedef PrfLedSimple<PrfLedG> PrfLedGSimple;
typedef PrfLedSimple<PrfLedR> PrfLedRSimple;


class DpaTaskSimpleSerializerFactory : public ObjectFactory<DpaTask, std::istream>
{
public:
  DpaTaskSimpleSerializerFactory();
  std::unique_ptr<DpaTask> parse(const std::string& request);
  std::string serializeError(const std::string& err);
};
