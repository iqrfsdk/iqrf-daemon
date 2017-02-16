#include "PrfPulseMeter.h"
#include "IqrfLogging.h"

const std::string PrfPulseMeter::PRF_NAME("Pulsemeter");

const std::string STR_CMD_PULSE_READ("PULSE_READ");
const std::string STR_CMD_STORE_COUNTER("STORE_COUNTER");
const std::string STR_CMD_DISABLE_AUTO_SLEEP("DISABLE_AUTO_SLEEP");

#ifndef THERM_SIM
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

void PrfPulseMeter::readCountersCommand(const std::chrono::seconds& sec)
{
  setCmd(Cmd::READ_COUNTERS);
  typedef std::chrono::duration<unsigned long, std::ratio<2097, 1000>> milis2097;
  milis2097 ms2097 = std::chrono::duration_cast<milis2097>(sec);
  uint16_t tm = (uint16_t)ms2097.count();
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = tm & 0xff; //LB
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = tm >> 8;   //HB
  //TODO HWPID
}

void PrfPulseMeter::storeCountersCommand(CntNum cntNum, uint32_t value)
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

void PrfPulseMeter::disableAutosleepCommand(bool enable)
{
  setCmd(Cmd::DISABLE_AUTO_SLEEP);
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = (uint8_t)(enable ? 0 : 1);
}

void PrfPulseMeter::parseResponse(const DpaMessage& response)
{
#ifdef THERM_SIM
  m_cnt1 = response.DpaPacket().DpaResponsePacket_t.DpaMessage.PerThermometerRead_Response.SixteenthValue;
#else
  const uint8_t* p = response.DpaPacket().DpaRequestPacket_t.DpaMessage.Response.PData;

  m_thermometerType = p[0];
  
  uint16_t tmp = p[1];
  tmp <<= 8;
  tmp |= p[2];
  m_temperature = tmp * 0.0625;

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

//TT: typ teplomeru 0x01 interni teplomer IQRF modulu
//  Thi, Tlo : horni a spodni bajt teploty
//  vypocet : T(stC) = val * 0.0625,
//  zde 0x0190 * 0.0625 = 400 * 0, 0625 = 25stC
//  PWR : napajeci napeti(typ)
//  0x01 napeti baterii
//  PWRhi, PWRlo horni a spodni bajt napeti
//  vypocet : U(V) = (3 / 1023)*val
//  zde(3 / 1023) * 0x03ff = (3 / 1023) * 1023 = 3V
//  SUP : hodnota getSupplyVoltage(IQR OS), napajeni napeti na IQRF modulu
//  vypocet U(V) = 261.12 / (127 - val)
//  zde 261.12 / (127 - 0x28) = 261.12 / (127 - 40) = 3.001V
//  RSSI : sila signalu
//  vypocet RSSI[dBm] = RSSI_LEVEL – 130 zde 0x58 - 130 = 88 - 130 = -42dBm
//  CNTLEN : pocet bajtu pocitadel(cntlen, poc1, poc2, sumpoc) zde 0x0A = 10
//  CNTS : pocet pocitadel
//  zde 0x02 = 2 pocitadla
//  Pocitadlo 1 : 4 bajty, nejprve nejnizsi bajt
//  zde 0x22 0x0c 0x00 0x00 = 0x00000c22 = 3106
//  Pocitadlo 2 : 4 bajty, nejprve nejnižší bajt
//  Zde 0xc1 0x00 0x00 0x00 = 0x000000c1 = 193
//  CNTSUM : kontrolni soucet pocitadel
//  Pocatecni hodnota sum = 0xff
//  vypocet : sum = sum - (hodnota + 1) pres vsechny bajty pocitadel vcetne poctu pocitadel
//            (bajt 9 - 17 resp. 17 - 25)
//  Zde sum = 0xFF - (0x02 + 0x1) - (0x22 + 0x01) - (0x0c + 0x01) - (0x00 + 0x01) - (0x00 + 0x01) -
//  (0xC1 + 0x01) - (0x00 + 0x01) - (0x00 + 0x01) - (0x00 + 0x01) = 0x17
//  DATASUM : kontrolni soucet dat
//  Pocatecni hodnota sum = 0xff
//  vypocet : sum = sum - (hodnota + 1) pres vsechny bajty(bajt 0 - 18, resp. 8 - 26)
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
  else if(STR_CMD_STORE_COUNTER == command)
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

  //TODO
  rapidjson::Value v;
  v = getCounter(CntNum::CNT_1);
  doc.AddMember("Pulse", v, doc.GetAllocator());

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Status", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

