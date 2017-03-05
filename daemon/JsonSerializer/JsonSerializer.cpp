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

//////////////////////////////////////////
void parseRequestJson(DpaTask& dpaTask, rapidjson::Value& val)
{
  jutils::assertIsObject("", val);
  dpaTask.setAddress(jutils::getMemberAs<int>("Addr", val));
  dpaTask.parseCommand(jutils::getMemberAs<std::string>("Comd", val));
  dpaTask.setTimeout(jutils::getPossibleMemberAs<int>("Timeout", val, -1));
}

void encodeResponseJson(const DpaTask& dpaTask, rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc)
{
  rapidjson::Value v;
  v.SetString(dpaTask.getPrfName().c_str(),alloc);
  val.AddMember("Type", v, alloc);

  v = dpaTask.getAddress();
  val.AddMember("Addr", v, alloc);

  v.SetString(dpaTask.encodeCommand().c_str(), alloc);
  val.AddMember("Comd", v, alloc);

  if (dpaTask.getTimeout() > 0) {
    v.SetInt(dpaTask.getTimeout());
    val.AddMember("Timeout", v, alloc);
  }

}

//-------------------------------
PrfRawJson::PrfRawJson(rapidjson::Value& val)
{
  jutils::assertIsObject("", val);
  std::string buf = jutils::getMemberAs<std::string>("Request", val);
  setTimeout(jutils::getPossibleMemberAs<int>("Timeout", val, -1));

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

    v.SetString(ostr.str().c_str(), doc.GetAllocator());
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
  parseRequestJson(*this, val);
}

std::string PrfThermometerJson::encodeResponse(const std::string& errStr) const
{
  Document doc;
  doc.SetObject();

  encodeResponseJson(*this, doc, doc.GetAllocator());

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
PrfIoJson::PrfIoJson(rapidjson::Value& val)
{
  parseRequestJson(*this, val);
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

  encodeResponseJson(*this, doc, doc.GetAllocator());

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
DpaTaskJsonSerializerFactory::DpaTaskJsonSerializerFactory()
{
  registerClass<PrfRawJson>(PrfRaw::PRF_NAME);
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
  registerClass<PrfIoJson>(PrfIo::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskJsonSerializerFactory::parseRequest(const std::string& request)
{
  std::unique_ptr<DpaTask> obj;
  try {
    Document doc;
    jutils::parseString(request, doc);

    jutils::assertIsObject("", doc);
    std::string perif = jutils::getMemberAs<std::string>("Type", doc);

    obj = createObject(perif, doc);
    m_lastError = "OK";
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return std::move(obj);
}

std::string DpaTaskJsonSerializerFactory::getLastError() const
{
  return m_lastError;
}

