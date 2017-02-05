#include "PrfPulseMeter.h"
#include "IqrfLogging.h"

const std::string PrfPulseMeter::PRF_NAME("Pulsemeter");

const std::string STR_CMD_PULSE_READ("PULSE_READ");

PrfPulseMeter::PrfPulseMeter()
  :DpaTask(PRF_NAME, PNUM_THERMOMETER) //TODO - just test now on ordinary node
{
  setCmd(Cmd::PULSE_READ);
}

PrfPulseMeter::PrfPulseMeter(uint16_t address)
  : DpaTask(PRF_NAME, PNUM_THERMOMETER) //TODO - just test now on ordinary node
{
  setAddress(address);
  setCmd(Cmd::PULSE_READ);
}

PrfPulseMeter::~PrfPulseMeter()
{
}

PrfPulseMeter::Cmd PrfPulseMeter::getCmd() const
{
  return m_cmd;
}

void PrfPulseMeter::setCmd(PrfPulseMeter::Cmd cmd)
{
  m_cmd = cmd;
  setPcmd((uint8_t)m_cmd);
}

void PrfPulseMeter::parseResponse(const DpaMessage& response)
{
  //TODO - just test now on ordinary node
  m_pulse = response.DpaPacket().DpaResponsePacket_t.DpaMessage.PerThermometerRead_Response.SixteenthValue;
}

void PrfPulseMeter::parseCommand(const std::string& command)
{
  if (STR_CMD_PULSE_READ == command)
    setCmd(Cmd::PULSE_READ);
  else
    THROW_EX(std::logic_error, "Invalid command: " << PAR(command));
}

const std::string& PrfPulseMeter::encodeCommand() const
{
  switch (getCmd()) {
  case Cmd::PULSE_READ:
    return STR_CMD_PULSE_READ;
  default:
    THROW_EX(std::logic_error, "Invalid command: " << NAME_PAR(command, (uint8_t)getCmd()));
  }
}

///////////////////////
PrfPulseMeterJson::PrfPulseMeterJson()
  :PrfPulseMeter()
{
}

PrfPulseMeterJson::PrfPulseMeterJson(uint16_t address)
  :PrfPulseMeter(address)
{
}

PrfPulseMeterJson::PrfPulseMeterJson(const PrfPulseMeterJson& o)
  : PrfPulseMeter(o)
  , m_common(*this)
{
}

PrfPulseMeterJson::PrfPulseMeterJson(rapidjson::Value& val)
{
}

std::string PrfPulseMeterJson::encodeResponse(const std::string& errStr) const
{
  using namespace rapidjson;
  
  Document doc;
  doc.SetObject();
  auto& alloc = doc.GetAllocator();

  m_common.encodeResponseJson(doc, alloc);

  rapidjson::Value v;
  v = getPulse();
  doc.AddMember("Pulse", v, doc.GetAllocator());

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Status", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

