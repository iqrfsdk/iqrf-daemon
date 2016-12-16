#include "IqrfLogging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <vector>
#include <utility>
#include <stdexcept>

using namespace rapidjson;

namespace jutils
{
  inline void parseJsonFile(const std::string& fname, rapidjson::Document& json)
  {
    //TODO using istream is slower according http://rapidjson.org/md_doc_stream.html
    std::ifstream ifs(fname);
    if (!ifs.is_open()) {
      THROW_EX(std::logic_error, "Cannot open: " << PAR(fname));
    }

    IStreamWrapper isw(ifs);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  inline void parseIstream(std::istream& istr, rapidjson::Document& json)
  {
    IStreamWrapper isw(istr);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  inline void parseString(const std::string& str, rapidjson::Document& json)
  {
    StringStream s(str.data());
    Document doc;
    json.ParseStream(s);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  //////////////////////////////////////
  template<typename T>
  inline void assertIs(const std::string& name, const rapidjson::Value& v) {
    if (!v.Is<T>())
      THROW_EX(std::logic_error, "Expected: " << typeid(T).name() << ", detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  template<>
  inline void assertIs<std::string>(const std::string& name, const rapidjson::Value& v) {
    if (!v.IsString())
      THROW_EX(std::logic_error, "Expected: " << typeid(std::string).name() << ", detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  inline void assertIsObject(const std::string& name, const rapidjson::Value& v)
  {
    if (!v.IsObject())
      THROW_EX(std::logic_error, "Expected: Json Object, detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  inline void assertIsArray(const std::string& name, const rapidjson::Value& v)
  {
    if (!v.IsArray())
      THROW_EX(std::logic_error, "Expected: Json Array, detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  //////////////////////////////////////
  inline const rapidjson::Value::ConstMemberIterator getMember(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const rapidjson::Value::ConstMemberIterator m = jsonValue.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd()) {
      THROW_EX(std::logic_error, "Expected member: " << PAR(name));
    }
    return m;
  }

  template<typename T>
  inline T getMemberAs(const std::string& name, const rapidjson::Value& v) {
    const auto m = getMember(name, v);
    assertIs<T>(name, m->value);
    return m->value.Get<T>();
  }

  template<>
  inline std::string getMemberAs<std::string>(const std::string& name, const rapidjson::Value& v) {
    const auto m = getMember(name, v);
    assertIs<std::string>(name, m->value);
    return std::string(m->value.GetString());
  }

  inline const rapidjson::Value& getMemberAsObject(const std::string& name, const rapidjson::Value& v)
  {
    const auto m = getMember(name, v);
    assertIsObject(name, m->value);
    return m->value;
  }

  //////////////////////////////////////
  template<typename T>
  inline T getPossibleMemberAs(const std::string& name, const rapidjson::Value& v, T defaultVal) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return defaultVal;
    }
    checkIs<T>(m->value);
    return m->value.Get<T>();
  }

  template<typename T>
  inline std::vector<T> getPossibleMemberAsVector(const std::string& name, const rapidjson::Value& v, std::vector<T> defaultVal = std::vector<T>())
  {
    const auto m = v.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd())
      return defaultVal;

    const rapidjson::Value& vct = m->value;
    checkIsArray(name, vct);
    defaultVal.clear();

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      checkIs<T>(name, *itr);
      defaultVal.push_back(itr->Get<T>());
    }

    return defaultVal;
  }

} //jutils
