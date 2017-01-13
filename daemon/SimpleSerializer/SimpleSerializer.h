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
    parseRequestSimple(*this, parseTokens(*this, istr));
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


class DpaTaskSimpleSerializerFactory : public ObjectFactory<DpaTask, std::istream>, public ISerializer
{
public:
  virtual ~DpaTaskSimpleSerializerFactory() {}
  DpaTaskSimpleSerializerFactory();

  std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
  std::string getLastError() const override;
private:
  std::string m_lastError;
};
