#pragma once

#include "ISerializer.h"
#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "PlatformDep.h"
#include <memory>
#include <string>

void parseRequestSimple(DpaTask& dpaTask, std::istream& istr);
void encodeResponseSimple(const DpaTask & dt, std::ostream& ostr);

class PrfRawSimple : public DpaRawTask
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
    parseRequestSimple(istr, *this);
  }

  virtual ~PrfLedSimple() {}

  std::string encodeResponse(const std::string& errStr) const override
  {
    std::ostringstream ostr;
    encodeResponseSimple(*this, ostr);
    ostr << " " << L::getLedState() << " " << errStr;
    return ostr.str();
  }
};

typedef PrfLedSimple<PrfLedG> PrfLedGSimple;
typedef PrfLedSimple<PrfLedR> PrfLedRSimple;


class DpaTaskSimpleSerializerFactory : public ObjectFactory<DpaTask, std::istream>, public ISerializer
{
public:
  DpaTaskSimpleSerializerFactory();

  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string getLastError() const override;
};
