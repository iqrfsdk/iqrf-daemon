#include "JsonSerializer.h"
#include "DpaTransactionTask.h"
#include "IqrfLogging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

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
//void parseRequestCommonJson(std::istream& istr, DpaTask& dpaTask)
//{
//  int address = -1;
//  std::string command;
//
//  istr >> address >> command;
//
//  dpaTask.setAddress(address);
//  dpaTask.parseCommand(command);
//}
//
//void encodeResponseCommonJson(std::ostream& ostr, DpaTask & dt)
//{
//  ostr << dt.getPrfName() << " " << dt.getAddress() << " " << dt.encodeCommand() << " ";
//}

void JsonSerialized::parseRequestJson(const rapidjson::Document& doc)
{
  jutils::checkIsObject("", doc);
  m_dpaTask->setAddress(jutils::getMemberAsInt("naddr", doc));
  m_dpaTask->parseCommand(jutils::getMemberAsString("command", doc));
}

void JsonSerialized::encodeResponseJson(rapidjson::Document& doc, const std::string& errStr)
{
  //ostr << dt.getPrfName() << " " << dt.getAddress() << " " << dt.encodeCommand() << " ";
}

//-------------------------------
PrfThermometerJson::PrfThermometerJson()
{
  m_dpaTask = std::unique_ptr<DpaTask>(ant_new PrfThermometer());
}

PrfThermometerJson::~PrfThermometerJson()
{
}

void PrfThermometerJson::parseRequestJson(const rapidjson::Document& doc)
{
  JsonSerialized::parseRequestJson(doc);
}

void PrfThermometerJson::encodeResponseJson(rapidjson::Document& doc, const std::string& errStr)
{
  JsonSerialized::encodeResponseJson(doc, errStr);
  //ostr << " " << getFloatTemperature();
}

///////////////////////////////////////////
DpaTaskJsonSerializerFactory::DpaTaskJsonSerializerFactory()
{
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  //registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  //registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
}

std::unique_ptr<DpaTask> DpaTaskJsonSerializerFactory::parse(const std::string& request)
{
  std::unique_ptr<Document> doc(ant_new Document);
  jutils::parseString(request, *doc);

  jutils::checkIsObject("", *doc);
  std::string perif = jutils::getMemberAsString("pnum", *doc);

  auto jsonSerialized = createObject(perif);
  jsonSerialized->parseRequestJson(*doc);
  return jsonSerialized->moveDpaTask();
}

#if 0
try {
  rapidjson::Document doc;
  JsonUtils::ParseJsonFile(fname, doc);

  JsonUtils::checkIsObject("", doc);
  m_name = JsonUtils::getMemberAsString("name", doc);
  m_sname = JsonUtils::getMemberAsString("symbolicName", doc);
  m_namespace = JsonUtils::getMemberAsString("namespace", doc);
  m_relativePath = JsonUtils::getPossibleMemberAsString("relativePath", doc);

  m_ver = JsonUtils::getMemberAsString("version", doc);
  m_libname = m_name; //TODO name+ver
  m_policy = JsonUtils::getMemberAsInt("activationPolicy", doc);
  m_persistency = JsonUtils::getMemberAsInt("activationPersistency", doc);

  const char* componentStr("Component");
  const auto memberComponent = doc.FindMember(componentStr);

  m_hasActivate = false;
  m_hasDeactivate = false;
  m_hasModified = false;
  m_hasLoggerSupport = false;

  if (memberComponent != doc.MemberEnd()) {
    //m_isComponent = true;
    const rapidjson::Value& component = memberComponent->value;
    JsonUtils::checkIsObject(componentStr, component);

    m_componentId = m_namespace + "::" + m_name;
    m_implSharedObj = JsonUtils::getPossibleMemberAsString("implSharedObject", component);
    m_immediate = JsonUtils::getMemberAsBool("immediate", component);
    m_autoEnable = JsonUtils::getMemberAsBool("autoEnable", component);
    m_singleton = JsonUtils::getMemberAsBool("singleton", component);

    const char* prSerStr("ProvidedServices");
    const auto memberPrSer = doc.FindMember(prSerStr);
    if (memberPrSer != doc.MemberEnd()) {
      const rapidjson::Value& prSer = memberPrSer->value;
      JsonUtils::checkIsArray(prSerStr, prSer);

      for (auto itr = prSer.Begin(); itr != prSer.End(); ++itr)
      {
        std::vector<int> ver = JsonUtils::getPossibleMemberAsVectorInt("version", *itr);

        int vmajor(0), vminor(0), vmicro(0);
        if (ver.size() >= 3) {
          vmajor = ver[0];
          vminor = ver[1];
          vmicro = ver[2];
        }
        else if (ver.size() == 2) {
          vmajor = ver[0];
          vminor = ver[1];
        }
        else if (ver.size() == 1) {
          vmajor = ver[0];
        }

        m_providedServices.push_back(
          ProvidedService(
          JsonUtils::getMemberAsString("name", *itr),
          JsonUtils::getPossibleMemberAsString("relativePath", *itr),
          vmajor, vminor, vmicro
          )
          );
      }
    }

    //Logger support
    const char* logSupStr("LoggerSupport");
    if (JsonUtils::getPossibleMemberAsBool(logSupStr, doc)) {
      m_hasLoggerSupport = true;
      m_references.push_back(ComponentReferenceJson(
        "hps::ILoggerService",
        "",
        "hps::ILoggerService",
        "",                             // relative path to interface file
        ComponentReference::STATIC,
        ComponentReference::MULTIPLE,
        ComponentReference::OPTIONAL_REF
        ));
    }

    m_hasActivate = JsonUtils::getPossibleMemberAsBool("Activate", doc, true);
    m_hasDeactivate = JsonUtils::getPossibleMemberAsBool("Deactivate", doc, true);
    m_hasModified = JsonUtils::getPossibleMemberAsBool("Modified", doc, false);

    const char* rfSerStr("ReferencedServices");
    const auto memberRfSer = doc.FindMember(rfSerStr);
    if (memberRfSer != doc.MemberEnd()) {
      const rapidjson::Value& rfSer = memberRfSer->value;
      JsonUtils::checkIsArray(prSerStr, rfSer);

      for (auto itr = rfSer.Begin(); itr != rfSer.End(); ++itr) {
        m_references.push_back(ComponentReferenceJson(*itr));
      }
    }

    const auto memberProps = doc.FindMember("Properties");
    if (memberProps != doc.MemberEnd()) {
      JsonProps::ParseJsonObject(memberProps->value, m_properties);
    }
  }
  else {
    THROW_EX(std::exception, "Missing Component part");
  }

}
catch (const BaseException& e)
{
  THROW_EX(std::exception, "Parse failed for file: " << PAR(fname) << std::endl << e << std::endl);
  throw e;
}
#endif