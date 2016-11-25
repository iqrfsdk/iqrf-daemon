#pragma once

#include "ObjectFactory.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include <memory>
#include <string>

class DpaTaskSimpleSerializer
{
public:
  DpaTaskSimpleSerializer()
  {}

  std::unique_ptr<DpaTask> getDpaTask() { return m_dpaTask; }
  
  virtual void parseRequest(std::istream& istr) = 0;
  virtual void encodeResponse(std::ostream& ostr) = 0;

protected:
  void parseRequestCommon(std::istream& istr)
  {
    istr >> m_perif >> m_address >> m_command;
  }

  std::unique_ptr<DpaTask> getDpaTask() { return m_dpaTask; }

  void encodeResponseCommon(std::ostream& ostr)
  {
    ostr << m_perif << " " << m_address << " " << m_command << " ";
  }

  std::string m_perif;
  int m_address;
  std::string m_command;
  std::unique_ptr<DpaTask> m_dpaTask;
};

class PrfThermometerSimpleSerializer : public DpaTaskSimpleSerializer
{
public:
  PrfThermometerSimpleSerializer()
    :DpaTaskSimpleSerializer()
    ,m_therm(nullptr)
  {}

  virtual ~PrfThermometerSimpleSerializer()
  {
    delete m_therm;
  }

  virtual void parseRequest(std::istream& istr)
  {
    parseRequestCommon(istr);
    m_therm = ant_new PrfThermometer(m_address, PrfThermometer::convertCommand(m_command));
    m_dpaTask = m_therm;
  }

  void encodeResponse(std::ostream& ostr)
  {
    encodeResponseCommon(ostr);
    ostr << " " << m_therm->getFloatTemperature();
  }

private:
  PrfThermometer* m_therm;
};

class PrfLedGSimpleSerializer : public DpaTaskSimpleSerializer
{
public:
  PrfLedGSimpleSerializer()
    :DpaTaskSimpleSerializer()
    , m_ledG(nullptr)
  {}

  virtual ~PrfLedGSimpleSerializer()
  {
    delete m_ledG;
  }

  virtual void parseRequest(std::istream& istr)
  {
    parseRequestCommon(istr);
    m_ledG = ant_new PrfLedG(m_address, PrfLed::convertCommand(m_command));
    m_dpaTask = m_ledG;
  }

  void encodeResponse(std::ostream& ostr)
  {
    encodeResponseCommon(ostr);
    ostr << " " << m_ledG->getLedState();
  }

private:
  PrfLedG* m_ledG;
};

class PrfLedRSimpleSerializer : public DpaTaskSimpleSerializer
{
public:
  PrfLedRSimpleSerializer()
    :DpaTaskSimpleSerializer()
    , m_ledR(nullptr)
  {}

  virtual ~PrfLedRSimpleSerializer()
  {
    delete m_ledR;
  }

  virtual void parseRequest(std::istream& istr)
  {
    parseRequestCommon(istr);
    m_ledR = ant_new PrfLedR(m_address, PrfLed::convertCommand(m_command));
    m_dpaTask = m_ledR;
  }

  void encodeResponse(std::ostream& ostr)
  {
    encodeResponseCommon(ostr);
    ostr << " " << m_ledR->getLedState();
  }

private:
  PrfLedR* m_ledR;
};

//class SimpleSerializerFactory
//{
//public:
//  typedef std::function<std::shared_ptr<DpaTask>(const std::string&)> CreateDpaTaskFunc;
//  
//  SimpleSerializerFactory();
//  virtual ~SimpleSerializerFactory();
//
//  void parseTask(const std::string& task) {
//
//  }
//
//  void encodeTask(const std::string& task) {
//
//  }
//
//private:
//  std::map<std::string, CreateDpaTaskFunc> m_creator;
//  //int handleScheduledRecord(const std::string& msg);
//};
