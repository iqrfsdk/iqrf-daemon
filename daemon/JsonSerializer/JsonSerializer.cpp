#include "JsonSerializer.h"
#include "DpaTransactionTask.h"
#include "IqrfLogging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

//TODO using istream is slower according http://rapidjson.org/md_doc_stream.html

using namespace rapidjson;

namespace jutils
{
  void parseJsonFile(const std::string& fname, rapidjson::Document& json)
  {
    std::ifstream ifs(fname);
    if (!ifs.is_open()) {
      THROW_EX(std::exception, "Cannot open: " << PAR(fname));
    }

    IStreamWrapper isw(ifs);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::exception, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  void parseIstream(std::istream& istr, rapidjson::Document& json)
  {
    IStreamWrapper isw(istr);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::exception, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  void parseString(const std::string& str, rapidjson::Document& json)
  {
    StringStream s(str.data());
    Document doc;
    json.ParseStream(s);

    if (json.HasParseError()) {
      THROW_EX(std::exception, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }


  void checkIsObject(const std::string& name, const rapidjson::Value& jsonValue)
  {
    if (!jsonValue.IsObject())
      THROW_EX(std::exception, "Expected: Json Object, detected: " << PAR(jsonValue.GetType()) << PAR(name));
  }

  void checkIsArray(const std::string& name, const rapidjson::Value& jsonValue)
  {
    if (!jsonValue.IsArray())
      THROW_EX(std::exception, "Expected: Json Array, detected: " << PAR(jsonValue.GetType()) << PAR(name));
  }

  void checkIsString(const std::string& name, const rapidjson::Value& jsonValue)
  {
    if (!jsonValue.IsString())
      THROW_EX(std::exception, "Expected: Json String, detected: " << PAR(jsonValue.GetType()) << PAR(name));
  }

  void checkIsInt(const std::string& name, const rapidjson::Value& jsonValue)
  {
    if (!jsonValue.IsInt())
      THROW_EX(std::exception, "Expected: Json Int, detected: " << PAR(jsonValue.GetType()) << PAR(name));
  }

  const rapidjson::Value::ConstMemberIterator getMember(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const rapidjson::Value::ConstMemberIterator m = jsonValue.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd()) {
      THROW_EX(std::exception, "Expected member: " << PAR(name));
    }
    return m;
  }

  const rapidjson::Value& getMemberAsObject(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const auto m = getMember(name, jsonValue);
    if (!m->value.IsObject())
      THROW_EX(std::exception, "Expected: Object, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value;
  }

  std::string getMemberAsString(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const auto m = getMember(name, jsonValue);
    if (!m->value.IsString())
      THROW_EX(std::exception, "Expected: Json String, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return std::string(m->value.GetString());
  }

  bool getMemberAsBool(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const auto m = getMember(name, jsonValue);
    if (!m->value.IsBool())
      THROW_EX(std::exception, "Expected: Json Bool, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value.GetBool();
  }

  int getMemberAsInt(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const auto m = getMember(name, jsonValue);
    if (!m->value.IsInt())
      THROW_EX(std::exception, "Expected: Json Int, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value.GetInt();
  }

  double getMemberAsDouble(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const auto m = getMember(name, jsonValue);
    if (!m->value.IsDouble())
      THROW_EX(std::exception, "Expected: Json Double, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value.GetDouble();
  }

  unsigned getMemberAsUint(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const auto m = getMember(name, jsonValue);
    if (!m->value.IsUint())
      THROW_EX(std::exception, "Expected: Json Uint, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value.GetUint();
  }

  bool getPossibleMemberAsBool(const std::string& name, const rapidjson::Value& jsonValue, bool defaultVal = false)
  {
    const auto m = jsonValue.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd()) {
      return defaultVal;
    }
    if (!m->value.IsBool())
      THROW_EX(std::exception, "Expected: Json Bool, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value.GetBool();
  }

  std::string getPossibleMemberAsString(const std::string& name, const rapidjson::Value& jsonValue, std::string defaultVal = "")
  {
    const auto m = jsonValue.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd()) {
      return defaultVal;
    }
    if (!m->value.IsString())
      THROW_EX(std::exception, "Expected: Json String, detected: " << PAR(name) << NAME_PAR(type, m->value.GetType()));
    return m->value.GetString();
  }

  std::vector<int> getPossibleMemberAsVectorInt(const std::string& name, const rapidjson::Value& jsonValue,
    std::vector<int> defaultVal = std::vector<int>())
  {
    const auto m = jsonValue.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd())
      return defaultVal;

    const rapidjson::Value& vct = m->value;
    checkIsArray(name, vct);
    defaultVal.clear();

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      checkIsInt(name, *itr);
      defaultVal.push_back(itr->GetInt());
    }

    return defaultVal;
  }

} //jutils

//////////////////////////////////////////
void parseRequestJson(DpaTask& dpaTask, rapidjson::Value& val)
{
  jutils::checkIsObject("", val);
  dpaTask.setAddress(jutils::getMemberAsInt("Addr", val));
  dpaTask.parseCommand(jutils::getMemberAsString("Comd", val));
}

void encodeResponseJson(DpaTask& dpaTask, rapidjson::Value& val, rapidjson::Document::AllocatorType& alloc)
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

void PrfThermometerJson::encodeResponseMessage(std::ostream& ostr, const std::string& errStr)
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
  ostr << buffer.GetString();
}

///////////////////////////////////////////
DpaTaskJsonSerializerFactory::DpaTaskJsonSerializerFactory()
{
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskJsonSerializerFactory::parse(const std::string& request)
{
  Document doc;
  jutils::parseString(request, doc);

  jutils::checkIsObject("", doc);
  std::string perif = jutils::getMemberAsString("Type", doc);

  auto obj = createObject(perif, doc);
  return std::move(obj);
}
