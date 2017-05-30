/**
 * Copyright 2015-2017 MICRORISC s.r.o.
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
#include <vector>
#include <map>
#include <set>
#include <cmath>

#define PNUM_SE 0x5E

struct ScaleParameters
{
  ScaleParameters() {}
  
  ScaleParameters(
  double sByte,
  int oByte,
  double sWord,
  int oWord,
  double sDWord,
  int oDWord
  )
    :scaleByte(sByte)
    ,offsetByte(oByte)
    ,scaleWord(sWord)
    ,offsetWord(oWord)
    ,scaleDWord(sDWord)
    ,offsetDWord(oDWord)
  {}

  double scaleByte = 0.0;
  int offsetByte = 0;
  double scaleWord = 0.0;
  int offsetWord = 0;
  double scaleDWord = 0.0;
  int offsetDWord = 0;
};

class Sensor
{
public:
  Sensor() {}

  Sensor(uint8_t sid, const std::string& sensorType, const std::string& units, const std::string& sensorName, ScaleParameters scaleParameters)
    :m_sid(sid)
    , m_sensorType(sensorType)
    , m_units(units)
    , m_retlen(-1)
    , m_sensorName(sensorName)
    , m_sp(scaleParameters)
  {
    if ((m_sid & 0x80) == 0)
      m_retlen = 2;
    else if ((m_sid & 0xE0) == 0x80)
      m_retlen = 1;
    else if ((m_sid & 0xE0) == 0xA0)
      m_retlen = 4;
    else if ((m_sid & 0xC0) == 0xC0)
      m_retlen = -1;
  }

  virtual ~Sensor() {}

  void scale(uint8_t& f, double val) const
  {
    double fd = m_sp.scaleByte * val + m_sp.offsetByte;
    if (fd > std::numeric_limits<uint8_t>::max()) {
      f = 0;
    }
    else {
      f = (uint8_t)floor(fd);
    }
  }

  void scale(uint16_t& f, double val) const
  {
    double fd = m_sp.scaleWord * val + m_sp.offsetWord;
    if (fd > std::numeric_limits<uint16_t>::max()) {
      f = 0;
    }
    else {
      f = (uint16_t)floor(fd);
    }
  }

  void scale(uint32_t& f, double val) const
  {
    double fd = m_sp.scaleDWord * val + m_sp.offsetDWord;
    if (fd > std::numeric_limits<uint32_t>::max()) {
      f = 0;
    }
    else {
      f = (uint32_t)floor(fd);
    }
  }

  void descale(double& val, uint8_t f) const
  {
    int fi = f;
    val = (fi - m_sp.offsetByte) / m_sp.scaleByte;
  }

  void descale(double& val, uint16_t f) const
  {
    int fi = f;
    val = (fi - m_sp.offsetWord) / m_sp.scaleWord;
  }

  void descale(double& val, uint32_t f) const
  {
    int fi = f;
    val = (fi - m_sp.offsetDWord) / m_sp.scaleDWord;
  }

  virtual const uint8_t* descale(const uint8_t* data)
  {
    if (m_retlen == 1) {

    }
    return 0;
  }

  int getSid() const { return m_sid; }
  const std::string& getType() const { return m_sensorType; }
  const std::string& getUnits() const { return m_units; }
  const std::string& getName() const { return m_sensorName; }

private:
  uint8_t m_sid = 0;
  std::string m_sensorType;
  std::string m_units;
  uint8_t m_retlen;
  std::string m_sensorName;
  ScaleParameters m_sp;
};

/*
class StdTemperature : public Sensor
{
public:
  StdTemperature();
  virtual ~StdTemperature();
private:
};

class StdCo2 : public Sensor
{
public:
  StdCo2();
  virtual ~StdCo2();
private:
};

class StdVoc : public Sensor
{
public:
  StdVoc();
  virtual ~StdVoc();
private:
};

class StdHumidity : public Sensor
{
public:
  StdHumidity();
  virtual ~StdHumidity();
private:
};

class StdBinary : public Sensor
{
public:
  StdBinary();
  virtual ~StdBinary();
private:
};
*/

///////////////////
class StdSensor
{
public:
  StdSensor() {}
  StdSensor(const std::string name, const std::string producer, uint16_t hwpid, std::vector<Sensor> sensors)
    :m_name(name)
    , m_producer(producer)
    , m_hwpid(hwpid)
    , m_sensors(sensors)
  {}

  virtual ~StdSensor() {}

  const std::string& getName() const { return m_name; }
  std::string getProducer() const { return m_producer; }
  uint16_t getHwpid() const { return m_hwpid; }

  //returns sensor according if index valid else nullptr
  const Sensor* getSensor(int index) const
  {
    if (index >=0 && index < m_sensors.size()) {
      return &m_sensors[index];
    }
    else {
      return nullptr;
    }
  }

  //returns index according name if name exists else -1
  int getSensor(const std::string& sensorName) const
  {
    int retval = -1;
    for (int i = 0; i < m_sensors.size(); i++) {
      if (m_sensors[i].getName() == sensorName) {
        retval = i;
        break;
      }
    }
    return retval;
  }
  
private:
  std::string m_name;
  std::string m_producer;
  uint16_t m_hwpid = 0;
  std::vector<Sensor> m_sensors;
};
////////////////////////

class StdSensorRepo
{
public:
  static StdSensorRepo& get() {
    static StdSensorRepo repo;
    return repo;
  }
  virtual ~StdSensorRepo() {}
  void addStdSensor(const StdSensor& stdSensor) {
    m_stdSensors.insert(std::make_pair(stdSensor.getName(), stdSensor));
  }
  
  const StdSensor* getSensor(const std::string& name)
  {
    auto found = m_stdSensors.find(name);
    if (found != m_stdSensors.end()) {
      return &found->second;
    }
    else
      return nullptr;
  }

private:
  StdSensorRepo() {}
  std::map<std::string, StdSensor> m_stdSensors;
};

//////////////////////////////////////////////
class PrfStdSen : public DpaTask
{
public:
  enum class Cmd {
    READ = 0,
    READT = 1,
    ENUM = 2,
  };

  static const std::string PRF_NAME;

  PrfStdSen();
  PrfStdSen(Cmd command);
  virtual ~PrfStdSen();

  void parseResponse(const DpaMessage& response) override;

  void parseCommand(const std::string& command) override;
  const std::string& encodeCommand() const override;

  void readCommand(std::vector<std::string> sensorNames);
  void readtCommand(std::vector<std::string> sensorNames);
  void enumCommand();

  Cmd getCmd() const;
  void setCmd(Cmd cmd);

  uint32_t getBitmap() { return m_bitmap; }
  void setBitmap(uint32_t bitmap) { m_bitmap = bitmap ? bitmap : 1; m_useBitmap = true; }
  void resetBitmap() { m_bitmap = 1; m_useBitmap = false; }

  void setStdSensor(const StdSensor* stdSensor) { m_stdSensor = stdSensor; }
  const StdSensor* getStdSensor() { return m_stdSensor; }

protected:
  const StdSensor* m_stdSensor = nullptr;

private:
  void selectSensors(const std::vector<std::string>& sensorNames);

  Cmd m_cmd = Cmd::READ;
  uint32_t m_bitmap = 1;
  bool m_useBitmap = true;
  std::basic_string<unsigned char> Writtendata; //TODO

  std::vector<std::pair<int,std::string>> m_required;
  std::set<int> m_selected;

};
