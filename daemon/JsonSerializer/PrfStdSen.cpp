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
  { 2, "CO2", "ppm", "CO2_1", { 1, 5, 15, 5, 0, 0 } },
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

void PrfStdSen::parseResponse(const DpaMessage& response)
{
  if (getCmd() == Cmd::READ) {
    if (m_bitmap == 0) {
      //1st sensor
      TDpaMessage& dpaMsg = m_request.DpaPacket().DpaResponsePacket_t.DpaMessage;
      const Sensor* sen = getStdSensor()->getSensor(0);
      //if (nullptr != sen) {
      //  sen->
      //}
      //dpaMsg.Response.PData[]
    }
  }
  else if (getCmd() == Cmd::READT) {
  }
  else if (getCmd() == Cmd::ENUM) {
  }
}

void PrfStdSen::readCommand(std::vector<std::string> sensorNames)
{
  selectSensors(sensorNames);

  setCmd(Cmd::READ);
  TDpaMessage& dpaMsg = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage;

  size_t maxSize = 0;
  if (m_bitmap != 0) {
    std::copy(&m_bitmap, &m_bitmap + 1, dpaMsg.Request.PData);
    maxSize = (DPA_MAX_DATA_LENGTH - sizeof(m_bitmap)) / 5;
  }

  //TODO WrittenData
  maxSize = 0;
  //if (directions.size() > maxSize)
  //  THROW_EX(std::logic_error, PAR(maxSize) << NAME_PAR(dirSize, directions.size()) << " too long");

  //maxSize = directions.size();
  //TDpaMessage& dpaMsg = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage;

  if (m_bitmap != 0) {
    m_request.SetLength(sizeof(TDpaIFaceHeader) + sizeof(m_bitmap) + 5 * maxSize);
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
  readCommand(std::vector<std::string>());
  setCmd(Cmd::READ); //ENUM is just pseudo-command
}

void PrfStdSen::selectSensors(const std::vector<std::string>& sensorNames)
{
  m_bitmap = 0;
  m_required.clear();
  m_selected.clear();

  if (m_stdSensor != nullptr) {
    for (auto const &name : sensorNames) {
      m_required.push_back(std::make_pair(m_stdSensor->getSensor(name), name));
    }

    for (auto &req : m_required) {
      if (req.first > -1) {
        m_selected.insert(req.first);
      }
    }

    for (auto &sel : m_selected) {
      if (sel < 32) {
        m_bitmap |= 1 << sel;
      }
    }
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

