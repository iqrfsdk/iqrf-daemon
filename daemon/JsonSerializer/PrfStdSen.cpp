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

#include "PrfStdSen.h"
#include "IqrfLogging.h"
#include <array>

const std::string PrfStdSen::PRF_NAME("std-sen");

// Commands
const std::string STR_CMD_READ("READ");
const std::string STR_CMD_READT("READT");
const std::string STR_CMD_ENUM("ENUM");

//TODO temporary till the remote repo ready
////////////////////////////////////////////////////////
const StdSensor MIC1("MIC1", "Microrisc", 1, {
  { 0, "Temperature", "Celsius", "Temperature1", { 0.5, 46, 0.0625, 4500, 0, 0 } }
});

const StdSensor PRO1("PRO1", "Protronic", 1, {
  { 0, "Temperature", "Celsius", "Temperature1", { 0.5, 46, 0.0625, 4500, 0, 0 } },
  { 2, "CO2", "ppm", "CO2_1", { 16, 5, 1, 5, 0, 0 } },
  { 0x80, "Humidity", "%", "Humidity1", { 0.5, 5, 0, 0, 0, 0 } }
});

class StdSensorRepoInitializer {
public:
  StdSensorRepoInitializer() {
    StdSensorRepo& repo = StdSensorRepo::get();
    repo.addStdSensor(MIC1);
    repo.addStdSensor(PRO1);
  }
};

static StdSensorRepoInitializer s_init;

/////////////////////////////////////////////
PrfStdSen::PrfStdSen()
  :DpaTask(PRF_NAME, PNUM_SE)
{
}

PrfStdSen::PrfStdSen(Cmd command)
  : DpaTask(PRF_NAME, PNUM_SE, 0, (uint8_t)command)
{
}

PrfStdSen::~PrfStdSen()
{
}

void PrfStdSen::parseResponseRead(const DpaMessage& response, bool typed)
{
  const uint8_t* data = response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
  unsigned datalen = response.Length() - sizeof(TDpaIFaceHeader) - 2;

  for (auto& idx : m_selected) {
    const Sensor* sen = m_stdSensor->getSensor(idx.first);
    if (sen != nullptr) {
      if (typed) {
        if (0 != datalen) {
          uint8_t type = *data++;
          if (type != sen->getSid()) {
            //TODO error
          }
        }
        else {
          //TODO error
        }
      }
      sen->descale(idx.second, data, datalen);
    }
    else {
      //TODO handle error
    }
  }
}

void PrfStdSen::parseResponse(const DpaMessage& response1)
{
  //TODO just 4 test
  std::vector<uint8_t> test{ 0x00, 0x00, 0x5E, 0x80, 0xFF, 0xFF, 0x00, 0x07, 0xFA, 0x12, 0x55, 0xC3, 0x37 };
  std::vector<uint8_t> test2{ 0x00, 0x00, 0x5E, 0x81, 0xFF, 0xFF, 0x00, 0x07, 0x01, 0xFA, 0x12, 0x02, 0x55, 0xC3, 0x80, 0x37 };
  std::vector<uint8_t> test3{ 0x00, 0x00, 0x5E, 0x80, 0xFF, 0xFF, 0x00, 0x07, 0x01, 0x02, 0x80 };
  DpaMessage response(test3.data(), test3.size());

  const uint8_t* data = response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
  unsigned datalen = response.Length() - sizeof(TDpaIFaceHeader) - 2;

  switch (getCmd()) {
  case Cmd::READ:
    parseResponseRead(response, false);
    break;
  case Cmd::READT:
    parseResponseRead(response, true);
    break;
  case Cmd::ENUM:
    if (datalen <= 32) {
      while (datalen > 0) {
        m_enumerated.push_back(*data++);
        datalen--;
      }
    }
    else {
      //TODO error
    }
    break;
  default:;
  }
}

void PrfStdSen::readCommand(std::vector<std::string> sensorNames)
{
  selectSensors(sensorNames);

  setCmd(Cmd::READ);
  TDpaMessage& dpaMsg = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage;

  size_t maxSize = 0;
  if (m_bitmap != 0) {
    m_bitmap = 0x11223344;
    std::copy((uint8_t*)(&m_bitmap), (uint8_t*)(&m_bitmap) + sizeof(m_bitmap), dpaMsg.Request.PData);
    maxSize = (DPA_MAX_DATA_LENGTH - sizeof(m_bitmap)) / (sizeof(uint8_t) + sizeof(uint32_t));
  }
  else {
    if (m_writeDataToSensors.size() > 0) {
      //TODO error cannot write data if bitmap = 0
    }
  }

  if (m_writeDataToSensors.size() <= maxSize) {
    maxSize = 0;
    uint8_t* data = dpaMsg.Request.PData + sizeof(m_bitmap);
    for (const auto& wd : m_writeDataToSensors) {
      *data++ = wd.first;
      std::copy((uint8_t*)(&wd.second), (uint8_t*)(&wd.second) + sizeof(wd.second), data);
      data += sizeof(wd.second);
      ++maxSize;
    }

  }
  else {
    //TODO error
    //  THROW_EX(std::logic_error, PAR(maxSize) << NAME_PAR(dirSize, directions.size()) << " too long");
  }

  if (m_bitmap != 0) {
    m_request.SetLength(sizeof(TDpaIFaceHeader) + sizeof(m_bitmap) + maxSize * (sizeof(uint8_t) + sizeof(uint32_t)));
  }
  else {
    m_request.SetLength(sizeof(TDpaIFaceHeader));
  }
}

void PrfStdSen::readtCommand(std::vector<std::string> sensorNames)
{
  readCommand(sensorNames);
  setCmd(Cmd::READT);
}

void PrfStdSen::enumCommand()
{
  m_bitmap = 0;
  setPcmd((uint8_t)Cmd::READ); //cmd ENUM just virtual
  TDpaMessage& dpaMsg = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage;
  std::copy(&m_bitmap, &m_bitmap + 1, dpaMsg.Request.PData);
  m_request.SetLength(sizeof(TDpaIFaceHeader) + sizeof(m_bitmap));
}

void PrfStdSen::prepareWriteDataToSensor(const std::string& sensorName, uint32_t data)
{
  if (nullptr != m_stdSensor) {
    uint8_t ix = m_stdSensor->getSensor(sensorName);
    if (ix >= 0) {
      m_writeDataToSensors.insert(std::make_pair(ix, data));
    }
    else {
      //TODO errror
    }
  }
  else {
    //TODO error
  }
  m_writeDataToSensors;
}

void PrfStdSen::selectSensors(const std::vector<std::string>& sensorNames)
{
  m_bitmap = 0;
  m_required.clear();
  m_selected.clear();
  bool use1st = false;

  if (m_stdSensor != nullptr) {

    if (sensorNames.size() == 0) { // no sensorNames explicitly required => require 1st by default
      m_required.push_back(std::make_pair(0, m_stdSensor->getSensor(0)->getName()));
    }
    else { // sensorNames are there, put them to required
      for (auto const &name : sensorNames) {
        m_required.push_back(std::make_pair(m_stdSensor->getSensor(name), name));
      }
    }

    // order them to selected
    for (auto &req : m_required) {
      if (req.first > -1) {
        m_selected.insert(std::make_pair(req.first, 0.0));
      }
    }

    // set bitmap according selected
    for (auto &sel : m_selected) {
      if (sel.first < 32) {
        m_bitmap |= 1 << sel.first;
      }
    }

    //handle special cases
    if (m_bitmap == 0) { // sensorNames has invalid names => nothing selected
      m_selected.insert(std::make_pair(0, 0.0)); //select 1st by default just to polute bitmap. Zero bitmap is reserved for ENUM cmd
      m_bitmap = 1;
    }

  }
  else {
    //TODO error
  }

}

////////////////////////////
PrfStdSen::Cmd PrfStdSen::getCmd() const
{
  return m_cmd;
}

void PrfStdSen::setCmd(PrfStdSen::Cmd cmd)
{
  m_cmd = cmd;
  setPcmd((uint8_t)m_cmd);
}

void PrfStdSen::parseCommand(const std::string& command)
{
  if (STR_CMD_READ == command)
    setCmd(Cmd::READ);
  else if (STR_CMD_READT == command)
    setCmd(Cmd::READT);
  else if (STR_CMD_ENUM == command)
    setCmd(Cmd::ENUM);
  else
    THROW_EX(std::logic_error, "Invalid command: " << PAR(command));
}

const std::string& PrfStdSen::encodeCommand() const
{
  switch (getCmd()) {
  case Cmd::READ: return STR_CMD_READ;
  case Cmd::READT: return STR_CMD_READT;
  case Cmd::ENUM: return STR_CMD_ENUM;
  default:
    THROW_EX(std::logic_error, "Invalid command: " << NAME_PAR(command, (int)getCmd()));
  }
}

