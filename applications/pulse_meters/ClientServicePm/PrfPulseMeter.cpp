/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#ifdef THERM_SIM
  m_request.SetLength(sizeof(TDpaIFaceHeader));
#else
  m_request.SetLength(sizeof(TDpaIFaceHeader) + 2);
#endif
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
  m_request.SetLength(sizeof(TDpaIFaceHeader) + 6);
}

void PrfPulseMeter::commandDisableAutosleep(bool enable)
{
  setCmd(Cmd::DISABLE_AUTO_SLEEP);
  m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = (uint8_t)(enable ? 0 : 1);
  m_request.SetLength(sizeof(TDpaIFaceHeader) + 1);
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
    const uint8_t* p = response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;

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

    //TODO use cnts for counters
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

    m_cntsum = p[18];
    m_datasum = p[19];

    uint8_t cntsum = 0xff;
    const uint8_t* pp = &p[9];
    for (int i = 0; i < m_cntlen-1; i++)
      cntsum -= pp[i] - 1;

    uint8_t datasum = 0xff;
    pp = &p[0];
    for (int i = 0; i < 19; i++)
      datasum -= pp[i] - 1;

    m_cntsum ^= cntsum;
    m_datasum ^= datasum;
#endif
  }
  break;

  case (uint8_t)Cmd::STORE_COUNTER:
  {
    m_storeCounterResult = response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0] != 0;
  }
  break;

  case (uint8_t)Cmd::DISABLE_AUTO_SLEEP:
  {
    m_disableAutosleepResult = response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0] != 0;
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

PrfPulseMeterJson::PrfPulseMeterJson(const PrfPulseMeterJson& o)
  :PrfCommonJson(o)
  ,PrfPulseMeter(o)
{
}

PrfPulseMeterJson::PrfPulseMeterJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);
}

void PrfPulseMeterJson::setNadr(uint16_t nadr)
{
  encodeHexaNum(m_nadr, nadr);
  this->setAddress(nadr);
  m_has_nadr = true;
}

std::string PrfPulseMeterJson::encodeResponse(const std::string& errStr)
{
  using namespace rapidjson;
  using namespace std::chrono;

  m_doc.RemoveAllMembers();

  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);

  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  uint8_t cmd = (uint8_t)getCmd();
  switch (cmd) {

  case (uint8_t)Cmd::READ_COUNTERS:
  {
    v = getThermometerType();
    m_doc.AddMember("ThermometerType", v, alloc);

    v = getTemperature();
    m_doc.AddMember("Temperature", v, alloc);

    v = getPowerVoltageType();
    m_doc.AddMember("PowerVoltageType", v, alloc);

    v = getPowerVoltage();
    m_doc.AddMember("PowerVoltage", v, alloc);

    v = getIqrfSuplyVoltage();
    m_doc.AddMember("SupplyVoltage", v, alloc);

    v = getRssi();
    m_doc.AddMember("Rssi", v, alloc);

    v = getCnts();
    m_doc.AddMember("CountersNum", v, alloc);

    v = getCounter(PrfPulseMeterJson::CntNum::CNT_1);
    m_doc.AddMember("Counter1", v, alloc);

    v = getCounter(PrfPulseMeterJson::CntNum::CNT_2);
    m_doc.AddMember("Counter2", v, alloc);

    v = getCntSum();
    m_doc.AddMember("CountersCheckSum", v, alloc);

    v = getDataSum();
    m_doc.AddMember("DataCheckSum", v, alloc);

  }
  break;

  case (uint8_t)Cmd::STORE_COUNTER:
  {
    v = getStoreCounterResult();
    m_doc.AddMember("StoreCounterResult", v, alloc);
  }
  break;

  case (uint8_t)Cmd::DISABLE_AUTO_SLEEP:
  {
    v = getDisableAutosleepResult();
    m_doc.AddMember("DisableAutosleepResult", v, alloc);
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

  v.SetString(buf, alloc);
  m_doc.AddMember("Time", v, alloc);

  m_statusJ = errStr;
  return encodeResponseJsonFinal(*this);
}
