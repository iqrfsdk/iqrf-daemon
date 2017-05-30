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

class Sensor
{
public:
  Sensor() {}

  Sensor(uint8_t sid, const std::string& sensorType, const std::string& units, uint8_t retlen, const std::string& sensorName,
    double scale, int offset, double scale2, int offset2)
    :m_sid(sid)
    , m_sensorType(sensorType)
    , m_units(units)
    , m_retlen(retlen)
    , m_sensorName(sensorName)
    , m_scale(scale)
    , m_offset(offset)
    , m_scale2(scale2)
    , m_offset2(offset2)
  {
  }

  virtual ~Sensor() {}

  void scale(uint8_t& f, double val) const
  {
    double fd = m_scale * val + m_offset;
    if (fd > std::numeric_limits<uint8_t>::max()) {
      f = 0;
    }
    else {
      f = (uint8_t)floor(fd);
    }
  }

  void scale(uint16_t& f, double val) const
  {
    double fd = m_scale2 * val + m_offset2;
    if (fd > std::numeric_limits<uint16_t>::max()) {
      f = 0;
    }
    else {
      f = (uint16_t)floor(fd);
    }
  }

  void descale(double& val, uint8_t f) const
  {
    int fi = f;
    val = (fi - m_offset) / m_scale;
  }

  void descale(double& val, uint16_t f) const
  {
    int fi = f;
    val = (fi - m_offset2) / m_scale2;
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
  double m_scale = 0;
  int m_offset = 0;
  double m_scale2 = 0;
  int m_offset2 = 0;
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
  const Sensor* getSensor(int index)
  {
    if (index >=0 && index < m_sensors.size()) {
      return &m_sensors[index];
    }
    else {
      return nullptr;
    }
  }

  //returns index according name if name exists else -1
  int getSensor(const std::string& sensorName)
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
    m_stdSensors.insert(std::make_pair(stdSensor.getHwpid(), stdSensor));
  }
  const StdSensor& getSensor(int i);
private:
  StdSensorRepo() {}
  std::map<int, StdSensor> m_stdSensors;
};

class SelectedSensor
{

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

protected:

private:
  void selectSensors(const std::vector<std::string>& sensorNames);

  Cmd m_cmd = Cmd::READ;
  uint32_t m_bitmap = 1;
  bool m_useBitmap = true;
  std::basic_string<unsigned char> Writtendata; //TODO

  StdSensor m_stdSensor;

  std::vector<std::pair<int,std::string>> m_required;
  std::set<int> m_selected;

};
