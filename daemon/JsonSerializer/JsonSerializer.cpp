#include "LaunchUtils.h"
#include "JsonSerializer.h"
#include "DpaTransactionTask.h"
#include "IqrfLogging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "JsonUtils.h"
#include <vector>
#include <utility>
#include <stdexcept>

//TODO using istream is slower according http://rapidjson.org/md_doc_stream.html

using namespace rapidjson;

INIT_COMPONENT(ISerializer, JsonSerializer)

//////////////////////////////////////////
PrfCommonJson::PrfCommonJson(DpaTask& dpaTask)
  :m_dpaTask(dpaTask)
{
}

void PrfCommonJson::parseRequestJson(rapidjson::Value& val)
{
  jutils::assertIsObject("", val);
  m_dpaTask.setAddress(jutils::getMemberAs<int>("Addr", val));
  m_dpaTask.parseCommand(jutils::getMemberAs<std::string>("Comd", val));
  int tout = jutils::getPossibleMemberAs<int>("Timeout", val, -1);
  if (tout >= 0) {
    m_explicitTimeout = true;
    m_dpaTask.setTimeout(tout);
  }
}

void PrfCommonJson::encodeResponseJson(rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc) const
{
  rapidjson::Value v;
  v.SetString(m_dpaTask.getPrfName().c_str(),alloc);
  val.AddMember("Type", v, alloc);

  v = m_dpaTask.getAddress();
  val.AddMember("Addr", v, alloc);

  v.SetString(m_dpaTask.encodeCommand().c_str(), alloc);
  val.AddMember("Comd", v, alloc);

  if (m_explicitTimeout > 0) {
    v.SetInt(m_dpaTask.getTimeout());
    val.AddMember("Timeout", v, alloc);
  }
}

//-------------------------------
PrfRawJson::PrfRawJson(rapidjson::Value& val)
{
  jutils::assertIsObject("", val);
  std::string buf = jutils::getMemberAs<std::string>("Request", val);
  setTimeout(jutils::getPossibleMemberAs<int>("Timeout", val, -1));

  if (std::string::npos != buf.find_first_of('.')) {
    buf.replace(buf.begin(), buf.end(), '.', ' ');
    m_dotNotation = true;
  }
  std::istringstream istr(buf);

  uint8_t bbyte;
  int ival;
  int i = 0;

  while (i < MAX_DPA_BUFFER) {
    if (istr >> std::hex >> ival) {
      m_request.DpaPacket().Buffer[i] = (uint8_t)ival;
      i++;
    }
    else
      break;
  }
  m_request.SetLength(i);

}

std::string PrfRawJson::encodeResponse(const std::string& errStr) const
{
  Document doc;
  doc.SetObject();

  rapidjson::Value v;
  v.SetString(getPrfName().c_str(), doc.GetAllocator());
  doc.AddMember("Type", v, doc.GetAllocator());

  {
    std::ostringstream ostr;
    int len = m_request.Length();
    ostr << iqrf::TracerHexString((unsigned char*)m_request.DpaPacket().Buffer, m_request.Length(), true);

    std::string buf(ostr.str());

    if (m_dotNotation) {
      buf.replace(buf.begin(), buf.end(), ' ', '.');
      if (buf[buf.size() - 1] == '.')
        buf.pop_back();
    }

    v.SetString(buf.c_str(), doc.GetAllocator());
    doc.AddMember("Request", v, doc.GetAllocator());
  }

  {
    std::ostringstream ostr;
    int len = m_response.Length();
    ostr << iqrf::TracerHexString((unsigned char*)m_response.DpaPacket().Buffer, m_response.Length(), true);

    v.SetString(ostr.str().c_str(), doc.GetAllocator());
    doc.AddMember("Response", v, doc.GetAllocator());
  }

  if (getTimeout() >= 0) {
    v.SetInt(getTimeout());
    doc.AddMember("Timeout", v, doc.GetAllocator());
  }

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Result", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

//-------------------------------
PrfThermometerJson::PrfThermometerJson(rapidjson::Value& val)
{
  m_common.parseRequestJson(val);
}

std::string PrfThermometerJson::encodeResponse(const std::string& errStr) const
{
  Document doc;
  doc.SetObject();

  m_common.encodeResponseJson(doc, doc.GetAllocator());

  rapidjson::Value v;
  v = getFloatTemperature();
  doc.AddMember("Temperature", v, doc.GetAllocator());

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Status", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

//-------------------------------
PrfFrcJson::PrfFrcJson(rapidjson::Value& val)
{
  m_common.parseRequestJson(val);

  std::string frcCmd = jutils::getPossibleMemberAs<std::string>("FrcCmd", val, "");
  if (!frcCmd.empty()) {
    setFrcCommand(parseFrcCmd(frcCmd));
    m_predefinedFrcCommand = true;
  }
  else {
    std::string frcType = jutils::getMemberAs<std::string>("FrcType", val);
    uint8_t frcUser = (uint8_t)jutils::getMemberAs<int>("FrcUser", val);
    setFrcCommand(parseFrcType(frcType), frcUser);
  }
}

std::string PrfFrcJson::encodeResponse(const std::string& errStr) const
{
  Document doc;
  doc.SetObject();
  auto& alloc = doc.GetAllocator();

  m_common.encodeResponseJson(doc, doc.GetAllocator());

  rapidjson::Value v;

  if (m_predefinedFrcCommand) {
    v.SetString(PrfFrc::encodeFrcCmd((FrcCmd)getFrcCommand()).c_str(), alloc);
    doc.AddMember("FrcCmd", v, doc.GetAllocator());
  }
  else {
    v.SetString(PrfFrc::encodeFrcType((FrcType)getFrcType()).c_str(), alloc);
    doc.AddMember("FrcType", v, doc.GetAllocator());

    v = (int)getFrcUser();
    doc.AddMember("FrcUser", v, doc.GetAllocator());
  }

  std::ostringstream os;
  os.setf(std::ios::hex, std::ios::basefield);
  os.fill('0');

  switch (getFrcType()) {
  case FrcType::GET_BIT2:
  {
    for (int i = 1; i <= PrfFrc::FRC_MAX_NODE_BIT2; i++) {
      os << std::setw(2) << (int)getFrcData_bit2(i) << ' ';
    }
  }
  break;

  case FrcType::GET_BYTE:
  {
    for (int i = 1; i <= PrfFrc::FRC_MAX_NODE_BYTE; i++) {
      os << std::setw(2) << (int)getFrcData_Byte(i) << ' ';
    }
  }
  break;

  case FrcType::GET_BYTE2:
  {
    for (int i = 1; i <= PrfFrc::FRC_MAX_NODE_BYTE2; i++) {
      os << std::setw(4) << (int)getFrcData_Byte2(i) << ' ';
    }
  }
  break;
  default:
    ;
  }

  std::string values(os.str());
  values.pop_back();

  v.SetString(values.c_str(), doc.GetAllocator());
  doc.AddMember("Values", v, doc.GetAllocator());

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Status", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  writer.SetFormatOptions(kFormatSingleLineArray);
  doc.Accept(writer);
  return buffer.GetString();
}

//-------------------------------
PrfIoJson::PrfIoJson(rapidjson::Value& val)
{
  m_common.parseRequestJson(val);
  switch (getCmd()) {
  
  case PrfIo::Cmd::DIRECTION:
  {
    Port port = parsePort(jutils::getMemberAs<std::string>("Port", val));
    int bit = jutils::getMemberAs<int>("Bit", val);
    bool inp = jutils::getMemberAs<bool>("Inp", val);
    directionCommand(port, bit, inp);
    m_port = port;
    m_bit = bit;
    m_val = inp;
  }
  break;

  case PrfIo::Cmd::SET:
  {
    Port port = parsePort(jutils::getMemberAs<std::string>("Port", val));
    int bit = jutils::getMemberAs<int>("Bit", val);
    bool value = jutils::getMemberAs<bool>("Val", val);
    setCommand(port, bit, value);
    m_port = port;
    m_bit = bit;
    m_val = value;
  }
  break;

  case PrfIo::Cmd::GET:
  {
    Port port = parsePort(jutils::getMemberAs<std::string>("Port", val));
    int bit = jutils::getMemberAs<int>("Bit", val);
    m_port = port;
    m_bit = bit;
    getCommand();
  }
  break;

  default:
    ;
  }
}

std::string PrfIoJson::encodeResponse(const std::string& errStr) const
{
  //00 00 09 82 00 00 00 00 fb 49 40 00 08
  Document doc;
  doc.SetObject();
  rapidjson::Value v;

  m_common.encodeResponseJson(doc, doc.GetAllocator());

  switch (getCmd()) {

  case PrfIo::Cmd::DIRECTION:
  {
    v.SetString(encodePort(m_port).c_str(), doc.GetAllocator());
    doc.AddMember("Port", v, doc.GetAllocator());

    v = m_bit;
    doc.AddMember("Bit", v, doc.GetAllocator());

    v = m_val;
    doc.AddMember("Out", v, doc.GetAllocator());
  }
  break;

  case PrfIo::Cmd::SET:
  {
    v.SetString(encodePort(m_port).c_str(), doc.GetAllocator());
    doc.AddMember("Port", v, doc.GetAllocator());

    v = m_bit;
    doc.AddMember("Bit", v, doc.GetAllocator());

    v = m_val;
    doc.AddMember("Val", v, doc.GetAllocator());
  }
  break;

  case PrfIo::Cmd::GET:
  {
    v.SetString(encodePort(m_port).c_str(), doc.GetAllocator());
    doc.AddMember("Port", v, doc.GetAllocator());

    v = m_bit;
    doc.AddMember("Bit", v, doc.GetAllocator());

    v = getInput(m_port, m_bit);
    doc.AddMember("Val", v, doc.GetAllocator());
  }
  break;

  default:
    ;
  }


  //"Port\":\"PORTB\",\"Bit\":4,\"Out\"
  //v = getFloatTemperature();
  //doc.AddMember("Temperature", v, doc.GetAllocator());

  v.SetString(errStr.c_str(), doc.GetAllocator());
  doc.AddMember("Status", v, doc.GetAllocator());

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}



///////////////////////////////////////////
JsonSerializer::JsonSerializer()
  :m_name("Json")
{
  init();
}

JsonSerializer::JsonSerializer(const std::string& name)
  :m_name(name)
{
  init();
}

void JsonSerializer::init()
{
  registerClass<PrfRawJson>(PrfRaw::PRF_NAME);
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
  registerClass<PrfFrcJson>(PrfFrc::PRF_NAME);
  registerClass<PrfIoJson>(PrfIo::PRF_NAME);
}

std::unique_ptr<DpaTask> JsonSerializer::parseRequest(const std::string& request)
{
  std::unique_ptr<DpaTask> obj;
  try {
    Document doc;
    jutils::parseString(request, doc);

    jutils::assertIsObject("", doc);
    std::string perif = jutils::getMemberAs<std::string>("Type", doc);

    obj = createObject(perif, doc);
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return std::move(obj);
}

std::string JsonSerializer::getLastError() const
{
  return m_lastError;
}

