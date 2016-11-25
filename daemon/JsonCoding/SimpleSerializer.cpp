#include "JsonCoding.h"
#include "DpaTransactionTask.h"
//#include "DpaTask.h"
#include "IqrfLogging.h"

#if 0
JsonCoding::JsonCoding()
{
}

JsonCoding::~JsonCoding()
{
}

int JsonCoding::handleScheduledRecord(const std::string& msg)
{
  std::string jsonmsg;
/*
  TRC_DBG("==================================" << std::endl <<
    "Scheduled msg: " << std::endl << FORM_HEX(record.getTask().data(), record.getTask().size()));

  //get input message
  std::istringstream is(record.getTask());

  //parse input message
  std::string device;
  int address(-1);
  is >> device >> address;

  //to encode output message
  std::ostringstream os;

  //check delivered address
  if (address < 0) {
    os << MQ_ERROR_ADDRESS;
  }
  else if (device == NAME_Thermometer) {
    DpaThermometer temp(address);
    DpaTransactionTask trans(temp);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << temp;
    else
      os << trans.getErrorStr();
  }
  else if (device == NAME_PulseLedG) {
    DpaPulseLedG pulse(address);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else if (device == NAME_PulseLedR) {
    DpaPulseLedR pulse(address);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else {
    os << MQ_ERROR_DEVICE;
  }

  //sendMessageToMq(os.str());
  {
    std::lock_guard<std::mutex> lck(m_responseHandlersMutex);
    auto found = m_responseHandlers.find(record.getClientId());
    if (found != m_responseHandlers.end()) {
      TRC_DBG(NAME_PAR(Response, os.str()) << " has been passed to: " << NAME_PAR(ClinetId, record.getClientId()));
      found->second(os.str());
      TRC_DBG(NAME_PAR(Response, os.str()) << " has been processed by: " << NAME_PAR(ClinetId, record.getClientId()));
    }
    else {
      TRC_DBG("Response: " << os.str());
    }
  }

  */
  return 0;
}
#endif