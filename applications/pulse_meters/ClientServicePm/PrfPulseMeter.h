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

#pragma once

#include "DpaTask.h"
#include "JsonSerializer.h"
#include <chrono>

class PrfPulseMeter: public DpaTask
{
public:
  enum class Cmd {
    READ_COUNTERS = 0,
    STORE_COUNTER = 1,
    DISABLE_AUTO_SLEEP = 2
  };

  enum class FrcCmd {
    ALIVE = 0,
    ALIVE_STOP_AUTOSLEEP = 1,
    ALIVE_START_AUTOSLEEP = 2
  };

  enum class CntNum {
    CNT_1 = 1,
    CNT_2 = 2
  };

  static const std::string PRF_NAME;

  PrfPulseMeter();
  virtual ~PrfPulseMeter();

  void commandReadCounters(const std::chrono::seconds& sec);
  void commandStoreCounter(CntNum cntNum, uint32_t value);
  void commandDisableAutosleep(bool enable);

  void parseResponse(const DpaMessage& response) override;
  void parseCommand(const std::string& command) override;
  const std::string& encodeCommand() const override;

  uint8_t getThermometerType() const { return m_thermometerType; }
  double getTemperature() const { return m_temperature; }
  uint8_t getPowerVoltageType() const { return m_powerVoltageType; }
  double getPowerVoltage() const { return m_powerVoltage; }
  double getIqrfSuplyVoltage() const { return m_iqrfSuplyVoltage; }
  int getRssi() const { return m_rssi; }
  int getCntLen() const { return m_cntlen; }
  int getCnts() const { return m_cnts; }
  int getCounter(CntNum cntNum) const;
  uint8_t getCntSum() const { return m_cntsum; }
  uint8_t getDataSum() const { return m_datasum; }

  bool getStoreCounterResult() const { return m_storeCounterResult; }
  bool getDisableAutosleepResult() const { return m_disableAutosleepResult; }

  Cmd getCmd() const;

private:
  void setCmd(Cmd cmd);

  Cmd m_cmd = Cmd::READ_COUNTERS;

  uint8_t m_thermometerType = 1;
  double m_temperature = -273.15;

  uint8_t m_powerVoltageType = 1;
  double m_powerVoltage = 0.0;

  double m_iqrfSuplyVoltage = 0.0;
  int m_rssi = 0;
  int m_cntlen = 0;
  int m_cnts = 0;
  int m_cnt1 = 0;
  int m_cnt2 = 0;
  uint8_t m_cntsum = 0xff;
  uint8_t m_datasum = 0xff;

  bool m_storeCounterResult = false;
  bool m_disableAutosleepResult = false;
};

class PrfPulseMeterJson : public PrfPulseMeter, public PrfCommonJson
{
public:
  PrfPulseMeterJson();
  PrfPulseMeterJson(const PrfPulseMeterJson& o);
  PrfPulseMeterJson(const rapidjson::Value& val);
  void setNadr(uint16_t nadr);
  virtual ~PrfPulseMeterJson() {}
  std::string encodeResponse(const std::string& errStr) override;
};
