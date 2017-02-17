#include "PrfPulseMeter.h"
#include "IqrfLogging.h"

const std::string PrfPulseMeter::PRF_NAME("Pulsemeter");

const std::string STR_CMD_PULSE_READ("PULSE_READ");
const std::string STR_CMD_STORE_COUNTER("STORE_COUNTER");
const std::string STR_CMD_DISABLE_AUTO_SLEEP("DISABLE_AUTO_SLEEP");

#ifdef THERM_SIM
const uint8_t PNUM_PULSEMETER(PNUM_THERMOMETER); //just test now on ordinary node
#else
const uint8_t PNUM_PULSEMETER(0x22);
#endif

PrfPulseMeter::PrfPulseMeter()
  :DpaTask(PRF_NAME, PNUM_PULSEMETER)
{
  setCmd(Cmd::READ_COUNTERS);
}

PrfPulseMeter::PrfPulseMeter(uint16_t address)
  : DpaTask(PRF_NAME, PNUM_PULSEMETER)
{
  setAddress(address);
  setCmd(Cmd::READ_COUNTERS);
}

PrfPulseMeter::~PrfPulseMeter()
{
}

void PrfPulseMeter::commandReadCounters(const std::chrono::seconds& sec)
{
  setCmd(Cmd::READ_COUNTERS);
  typedef std::chrono::duration<unsigned long, std::ratio<2097, 1000>> milis2097;
  milis2097 ms2097 = std::chrono::duration_cast<milis2097>(sec);
  uint16_t tm = (uint16_t)ms2097.count();
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = tm & 0xff; //LB
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = tm >> 8;   //HB
  //TODO HWPID
}

void PrfPulseMeter::commandStoreCounter(CntNum cntNum, uint32_t value)
{
  setCmd(Cmd::STORE_COUNTER);
  uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
  p[0] = (uint8_t)cntNum;
  p[4] = (uint8_t)(value & 0xff);
  p[3] = (uint8_t)((value >>= 8) & 0xff);
  p[2] = (uint8_t)((value >>= 8) & 0xff);
  p[1] = (uint8_t)((value >>= 8) & 0xff);

  uint8_t sum = 0xff - (p[1] + 0x01) - (p[1] + 0x01) - (p[1] + 0x01) - (p[1] + 0x01);
  p[5] = sum;
  //TODO HWPID
}

void PrfPulseMeter::commandDisableAutosleep(bool enable)
{
  setCmd(Cmd::DISABLE_AUTO_SLEEP);
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = (uint8_t)(enable ? 0 : 1);
}

void PrfPulseMeter::parseResponse(const DpaMessage& response)
{
  uint8_t cmd = (uint8_t)getCmd();
  switch (cmd) {

  case (uint8_t)Cmd::READ_COUNTERS:
  {
#ifdef THERM_SIM
    m_cnt1 = response.DpaPacket().DpaResponsePacket_t.DpaMessage.PerThermometerRead_Response.SixteenthValue;
    m_cnt2 = m_cnt1;
    m_temperature = (double)m_cnt1 * 0.0625;
#else
    const uint8_t* p = response.DpaPacket().DpaRequestPacket_t.DpaMessage.Response.PData;

    m_thermometerType = p[0];

    uint16_t tmp = p[1];
    tmp <<= 8;
    tmp |= p[2];
    m_temperature = (double)tmp * 0.0625;

    m_powerVoltageType = p[3];
    tmp = p[4];
    tmp <<= 8;
    tmp |= p[5];
    m_powerVoltage = (tmp * 3.0) / 1023;

    m_iqrfSuplyVoltage = 261.12 / (127 - p[6]);

    m_rssi = -130 + p[7];

    m_cntlen = p[8];
    m_cnts = p[9];

    m_cnt1 = p[13];
    m_cnt1 <<= 8;
    m_cnt1 |= p[12];
    m_cnt1 <<= 8;
    m_cnt1 |= p[11];
    m_cnt1 <<= 8;
    m_cnt1 |= p[10];

    m_cnt2 = p[17];
    m_cnt2 <<= 8;
    m_cnt2 |= p[16];
    m_cnt2 <<= 8;
    m_cnt2 |= p[15];
    m_cnt2 <<= 8;
    m_cnt2 |= p[14];

    uint8_t cntsum = 0xff;
    const uint8_t* pp = &p[9];
    for (int i = 0; i < m_cntlen; i++)
      cntsum -= pp[i] + 1;

    uint8_t datasum = 0xff;
    pp = &p[0];
    for (int i = 0; i < 19; i++)
      datasum -= pp[i] + 1;

    m_cntsum ^= cntsum;
    m_datasum ^= datasum;
#endif
  }
  break;

  case (uint8_t)Cmd::STORE_COUNTER:
  {
    m_storeCounterResult = response.DpaPacket().DpaRequestPacket_t.DpaMessage.Response.PData[0] != 0;
  }
  break;

  case (uint8_t)Cmd::DISABLE_AUTO_SLEEP:
  {
    m_disableAutosleepResult = response.DpaPacket().DpaRequestPacket_t.DpaMessage.Response.PData[0] != 0;
  }
  break;

  default:
    THROW_EX(std::logic_error, "Invalid command: " << PAR(cmd));
  }

}

int PrfPulseMeter::getCounter(CntNum cntNum) const
{
  switch (cntNum) {
  case CntNum::CNT_1:
    return m_cnt1;
  case CntNum::CNT_2:
    return m_cnt2;
  default:
    THROW_EX(std::logic_error, "Invalid cntNum: " << NAME_PAR(cntNum, (uint8_t)cntNum));
  }
}

void PrfPulseMeter::parseCommand(const std::string& command)
{
  if (STR_CMD_PULSE_READ == command)
    setCmd(Cmd::READ_COUNTERS);
  else if (STR_CMD_STORE_COUNTER == command)
    setCmd(Cmd::STORE_COUNTER);
  else if (STR_CMD_DISABLE_AUTO_SLEEP == command)
    setCmd(Cmd::DISABLE_AUTO_SLEEP);
  else
    THROW_EX(std::logic_error, "Invalid command: " << PAR(command));
}

const std::string& PrfPulseMeter::encodeCommand() const
{
  switch (getCmd()) {
  case Cmd::READ_COUNTERS:
    return STR_CMD_PULSE_READ;
  case Cmd::STORE_COUNTER:
    return STR_CMD_STORE_COUNTER;
  case Cmd::DISABLE_AUTO_SLEEP:
    return STR_CMD_DISABLE_AUTO_SLEEP;
  default:
    THROW_EX(std::logic_error, "Invalid command: " << NAME_PAR(command, (uint8_t)getCmd()));
  }
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

///////////////////////
PrfPulseMeterJson::PrfPulseMeterJson()
  :PrfPulseMeter()
{
}

PrfPulseMeterJson::PrfPulseMeterJson(uint16_t address)
  : PrfPulseMeter(address)
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
  using namespace std::chrono;

  Document doc;
  doc.SetObject();
  auto& alloc = doc.GetAllocator();
  rapidjson::Value v;

  m_common.encodeResponseJson(doc, alloc);

  uint8_t cmd = (uint8_t)getCmd();
  switch (cmd) {

  case (uint8_t)Cmd::READ_COUNTERS:
  {
    v = getThermometerType();
    doc.AddMember("ThermometerType", v, doc.GetAllocator());

    v = getTemperature();
    doc.AddMember("Temperature", v, doc.GetAllocator());

    v = getPowerVoltageType();
    doc.AddMember("PowerVoltageType", v, doc.GetAllocator());

    v = getPowerVoltage();
    doc.AddMember("PowerVoltage", v, doc.GetAllocator());

    v = getIqrfSuplyVoltage();
    doc.AddMember("SupplyVoltage", v, doc.GetAllocator());

    v = getRssi();
    doc.AddMember("Rssi", v, doc.GetAllocator());

    v = getCnts();
    doc.AddMember("CountersNum", v, doc.GetAllocator());

    v = getCounter(PrfPulseMeterJson::CntNum::CNT_1);
    doc.AddMember("Counter1", v, doc.GetAllocator());

    v = getCounter(PrfPulseMeterJson::CntNum::CNT_2);
    doc.AddMember("Counter2", v, doc.GetAllocator());

    v = getCntSum();
    doc.AddMember("CountersCheckSum", v, doc.GetAllocator());

    v = getDataSum();
    doc.AddMember("DataCheckSum", v, doc.GetAllocator());

  }
  break;

  case (uint8_t)Cmd::STORE_COUNTER:
  {
    v = getStoreCounterResult();
    doc.AddMember("StoreCounterResult", v, doc.GetAllocator());
  }
  break;

  case (uint8_t)Cmd::DISABLE_AUTO_SLEEP:
  {
    v = getDisableAutosleepResult();
    doc.AddMember("DisableAutosleepResult", v, doc.GetAllocator());
  }
  break;

  default:
    THROW_EX(std::logic_error, "Invalid command: " << PAR(cmd));
  }

  //TODO time from response or transaction?
  time_t tt = system_clock::to_time_t(system_clock::now());
  std::tm* timeinfo = localtime(&tt);
  timeinfo = localtime(&tt);
  std::tm timeStr = *timeinfo;
  char buf[80];
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &timeStr);

  v.SetString(buf, doc.GetAllocator());
  doc.AddMember("Time", v, doc.GetAllocator());

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Status", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

