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
  //dpaTask.setAddress(jutils::getMemberAsInt("Addr", val));
  //dpaTask.parseCommand(jutils::getMemberAsString("Comd", val));
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
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskJsonSerializerFactory::parseRequest(const std::string& request)
{
  Document doc;
  jutils::parseString(request, doc);

  jutils::assertIsObject("", doc);
  //std::string perif = jutils::getMemberAsString("Type", doc);
  std::string perif = jutils::getMemberAs<std::string>("Type", doc);

  auto obj = createObject(perif, doc);
  return std::move(obj);
}

std::string DpaTaskJsonSerializerFactory::getLastError() const
{
  return "OK";
}

