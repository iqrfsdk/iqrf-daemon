/**
 * Copyright 2016-2017 MICRORISC s.r.o.
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

#pragma once

#include "IqrfLogging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <vector>
#include <utility>
#include <stdexcept>

namespace jutils
{
  inline void parseJsonFile(const std::string& fname, rapidjson::Document& json)
  {
    //TODO using istream is slower according http://rapidjson.org/md_doc_stream.html
    std::ifstream ifs(fname);
    if (!ifs.is_open()) {
      THROW_EX(std::logic_error, "Cannot open: " << PAR(fname));
    }

    rapidjson::IStreamWrapper isw(ifs);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  inline void parseIstream(std::istream& istr, rapidjson::Document& json)
  {
    rapidjson::IStreamWrapper isw(istr);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  inline void parseString(const std::string& str, rapidjson::Document& json)
  {
    rapidjson::StringStream s(str.data());
    rapidjson::Document doc;
    json.ParseStream(s);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  //////////////////////////////////////
  template<typename T>
  inline void assertIs(const std::string& name, const rapidjson::Value& v) {
    if (!v.Is<T>()) {
      THROW_EX(std::logic_error, "Expected: " << typeid(T).name() << ", detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
    }
  }

  template<>
  inline void assertIs<std::string>(const std::string& name, const rapidjson::Value& v) {
    if (!v.IsString()) {
      THROW_EX(std::logic_error, "Expected: " << typeid(std::string).name() << ", detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
    }
  }

  inline void assertIsObject(const std::string& name, const rapidjson::Value& v)
  {
    if (!v.IsObject()) {
      THROW_EX(std::logic_error, "Expected: Json Object, detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
    }
  }

  inline void assertIsArray(const std::string& name, const rapidjson::Value& v)
  {
    if (!v.IsArray()) {
      THROW_EX(std::logic_error, "Expected: Json Array, detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
    }
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

  template<typename T>
  inline std::vector<T> getMemberAsVector(const std::string& name, const rapidjson::Value& v)
  {
    std::vector<T> retval;
    const auto m = getMember(name, v);
    const rapidjson::Value& vct = m->value;
    assertIsArray(name, vct);

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      assertIs<T>(name, *itr);
      retval.push_back(itr->Get<T>());
    }

    return retval;
  }

  template<>
  inline std::vector<std::string> getMemberAsVector<std::string>(const std::string& name, const rapidjson::Value& v)
  {
    std::vector<std::string> retval;
    const auto m = getMember(name, v);
    const rapidjson::Value& vct = m->value;
    assertIsArray(name, vct);

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      assertIs<std::string>(name, *itr);
      retval.push_back(std::string(itr->GetString()));
    }

    return retval;
  }

  //////////////////////////////////////
  template<typename T>
  inline T getPossibleMemberAs(const std::string& name, const rapidjson::Value& v, T defaultVal) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return defaultVal;
    }
    assertIs<T>(name, m->value);
    return m->value.Get<T>();
  }

  template<>
  inline std::string getPossibleMemberAs<std::string>(const std::string& name, const rapidjson::Value& v, std::string defaultVal) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return defaultVal;
    }
    assertIs<std::string>(name, m->value);
    return std::string(m->value.GetString());
  }

  template<typename T>
  inline std::vector<T> getPossibleMemberAsVector(const std::string& name, const rapidjson::Value& v, std::vector<T> defaultVal = std::vector<T>())
  {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return defaultVal;
    }

    const rapidjson::Value& vct = m->value;
    assertIsArray(name, vct);
    defaultVal.clear();

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      assertIs<T>(name, *itr);
      defaultVal.push_back(itr->Get<T>());
    }

    return defaultVal;
  }

} //jutils
