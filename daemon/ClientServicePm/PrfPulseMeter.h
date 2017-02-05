#pragma once

#include "DpaTask.h"
#include "JsonSerializer.h"

class PrfPulseMeter: public DpaTask
{
public:
  enum class Cmd {
    PULSE_READ = 0
  };

  static const std::string PRF_NAME;

  PrfPulseMeter();
  PrfPulseMeter(uint16_t address);
  virtual ~PrfPulseMeter();

  void parseResponse(const DpaMessage& response) override;
  void parseCommand(const std::string& command) override;
  const std::string& encodeCommand() const override;

  Cmd getCmd() const;
  void setCmd(Cmd cmd);

  uint16_t getPulse() const { return m_pulse; }

private:
  Cmd m_cmd = Cmd::PULSE_READ;
  uint16_t m_pulse = 0;
};

class PrfPulseMeterJson : public PrfPulseMeter
{
public:
  PrfPulseMeterJson();
  PrfPulseMeterJson(const PrfPulseMeterJson& o);
  PrfPulseMeterJson(uint16_t address);
  explicit PrfPulseMeterJson(rapidjson::Value& val);
  virtual ~PrfPulseMeterJson() {}
  std::string encodeResponse(const std::string& errStr) const  override;
private:
  PrfCommonJson m_common = *this;
};

