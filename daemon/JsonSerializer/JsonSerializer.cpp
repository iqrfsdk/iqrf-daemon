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
}

//-------------------------------
PrfRawJson::PrfRawJson(rapidjson::Value& val)
{
  jutils::assertIsObject("", val);
  std::string buf = jutils::getMemberAs<std::string>("Request", val);

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

  std::ostringstream ostr;
  int len = m_response.Length();
  ostr << iqrf::TracerHexString((unsigned char*)m_response.DpaPacket().Buffer, m_response.Length(), true);

  v.SetString(ostr.str().c_str(), doc.GetAllocator());
  doc.AddMember("Response", v, doc.GetAllocator());

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

///////////////////////////////////////////
DpaTaskJsonSerializerFactory::DpaTaskJsonSerializerFactory()
{
  registerClass<PrfRawJson>(DpaRawTask::PRF_NAME);
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskJsonSerializerFactory::parseRequest(const std::string& request)
{
  try {
    Document doc;
    jutils::parseString(request, doc);

    jutils::assertIsObject("", doc);
    std::string perif = jutils::getMemberAs<std::string>("Type", doc);

    auto obj = createObject(perif, doc);
    return std::move(obj);
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  m_lastError = "OK";
}

std::string DpaTaskJsonSerializerFactory::getLastError() const
{
  return m_lastError;
}

