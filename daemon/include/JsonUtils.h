/*
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

 /// \namespace jutils
 /// \brief Rapid Json values manipulation functions
namespace jutils
{
  /// \brief Parse file with Json content
  /// \param [in] fname file name
  /// \param [out] json value to hold parsed result
  /// \throws std::logic_error in case of parse error
  /// \details
  /// File fname is parsed to json value by rapidjson functions, In case of file open error or json syntax error
  /// the exception std::logic_error is thrown
  inline void parseJsonFile(const std::string& fname, rapidjson::Document& json)
  {
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

  /// \brief Parse stream with Json content
  /// \param [in] istr stream object
  /// \param [out] json value to hold parsed result
  /// \throws std::logic_error in case of parse error
  /// \details
  /// Stream content is taken and parsed to json value by rapidjson functions, In case of json syntax error
  /// the exception std::logic_error is thrown
  inline void parseIstream(std::istream& istr, rapidjson::Document& json)
  {
    rapidjson::IStreamWrapper isw(istr);
    json.ParseStream(isw);

    if (json.HasParseError()) {
      THROW_EX(std::logic_error, "Json parse error: " << NAME_PAR(emsg, json.GetParseError()) <<
        NAME_PAR(eoffset, json.GetErrorOffset()));
    }
  }

  /// \brief Parse string with Json content
  /// \param [in] str string object
  /// \param [out] json value to hold parsed result
  /// \throws std::logic_error in case of parse error
  /// \details
  /// String content is taken and parsed to json value by rapidjson functions, In case of json syntax error
  /// the exception std::logic_error is thrown
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

  /// \brief Assert json value holds T type
  /// \param [in] name json item name (key)
  /// \param [in] v json value to be checked
  /// \throws std::logic_error in case of assert failure
  /// \details
  /// Json value named name is checked if its content is template type T else the exception std::logic_error is thrown
  template<typename T>
  inline void assertIs(const std::string& name, const rapidjson::Value& v) {
    if (!v.Is<T>())
      THROW_EX(std::logic_error, "Expected: " << typeid(T).name() << ", detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  /// \brief Assert json value holds std::string type
  /// \param [in] name json item name (key)
  /// \param [in] v json value to be checked
  /// \throws std::logic_error in case of assert failure
  /// \details
  /// Json value named name is checked if its content is std::string
  /// else the exception std::logic_error is thrown.
  /// It is template specialization for std::string.
  template<>
  inline void assertIs<std::string>(const std::string& name, const rapidjson::Value& v) {
    if (!v.IsString())
      THROW_EX(std::logic_error, "Expected: " << typeid(std::string).name() << ", detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  /// \brief Assert json value holds Json object
  /// \param [in] name json item name (key)
  /// \param [in] v json value to be checked
  /// \throws std::logic_error in case of assert failure
  /// \details
  /// Json value named name is checked if its content is Json object {...} else the exception std::logic_error is thrown
  /// If not the exception std::logic_error is thrown
  inline void assertIsObject(const std::string& name, const rapidjson::Value& v)
  {
    if (!v.IsObject())
      THROW_EX(std::logic_error, "Expected: Json Object, detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  /// \brief Assert json value holds Json array
  /// \param [in] name json item name (key)
  /// \param [in] v json value to be checked
  /// \throws std::logic_error in case of assert failure
  /// \details
  /// Json value named name is checked if its content is Json object [...] else the exception std::logic_error is thrown
  inline void assertIsArray(const std::string& name, const rapidjson::Value& v)
  {
    if (!v.IsArray())
      THROW_EX(std::logic_error, "Expected: Json Array, detected: " << PAR(name) << NAME_PAR(type, v.GetType()));
  }

  /// \brief Get rapidjson member iterator
  /// \param [in] name json item name (key)
  /// \param [in] v json value to be checked
  /// \return rapidjson member iterator
  /// \throws std::logic_error in case the item doesn't exist
  /// \details
  /// Tries to find Json value named name and if found returns its iterator else the exception std::logic_error is thrown
  inline const rapidjson::Value::ConstMemberIterator getMember(const std::string& name, const rapidjson::Value& jsonValue)
  {
    const rapidjson::Value::ConstMemberIterator m = jsonValue.FindMember(name.c_str());
    if (m == jsonValue.MemberEnd()) {
      THROW_EX(std::logic_error, "Expected member: " << PAR(name));
    }
    return m;
  }

  /// \brief Get member with value of type T
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \return value of type T
  /// \throws std::logic_error in case the member doesn't exist or has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the template type T
  /// it returns the value else the exception std::logic_error is thrown
  template<typename T>
  inline T getMemberAs(const std::string& name, const rapidjson::Value& v) {
    const auto m = getMember(name, v);
    assertIs<T>(name, m->value);
    return m->value.Get<T>();
  }

  /// \brief Get member with value of type std::string
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \return value of type std::string
  /// \throws std::logic_error in case the member doesn't exist or has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the type std::string
  /// it returns the value else the exception std::logic_error is thrown
  /// It is template specialization for std::string
  template<>
  inline std::string getMemberAs<std::string>(const std::string& name, const rapidjson::Value& v) {
    const auto m = getMember(name, v);
    assertIs<std::string>(name, m->value);
    return std::string(m->value.GetString());
  }

  /// \brief Get member with value of type Json object
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \return value of type Json object
  /// \throws std::logic_error in case the member doesn't exist or has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds Json object {...}
  /// it returns the object else the exception std::logic_error is thrown
  inline const rapidjson::Value& getMemberAsObject(const std::string& name, const rapidjson::Value& v)
  {
    const auto m = getMember(name, v);
    assertIsObject(name, m->value);
    return m->value;
  }

  /// \brief Get member with value std::vector of values of template type T
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \return std::vector of values of type std::string
  /// \throws std::logic_error in case the member doesn't exist or has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds Json vector [...] of values of template type T
  /// it returns the vector else the exception std::logic_error is thrown.
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

  /// \brief Get member with value std::vector of values of type std::string
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \return std::vector of values of type std::string
  /// \throws std::logic_error in case the member doesn't exist or has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds Json vector [...] of values of types std::string
  /// it returns the vector else the exception std::logic_error is thrown.
  /// It is template specialization for std::string.
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

  /// \brief Get possible member with value of type T
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \param [in] defaultVal the value to be returned if the member is not found
  /// \return value of type T
  /// \throws std::logic_error in case the member has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the template type T
  /// it returns the value else the default one. If the member exists but has different type the exception std::logic_error is thrown.
  template<typename T>
  inline T getPossibleMemberAs(const std::string& name, const rapidjson::Value& v, T defaultVal) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return defaultVal;
    }
    assertIs<T>(name, m->value);
    return m->value.Get<T>();
  }

  /// \brief Get possible member with value of type std::string
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \param [in] defaultVal the value to be returned if the member is not found
  /// \return value of type std::string
  /// \throws std::logic_error in case the member has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the type std::string
  /// it returns the value else the default one. If the member exists but has different type the exception std::logic_error is thrown.
  /// It is template specialization for std::string.
  template<>
  inline std::string getPossibleMemberAs<std::string>(const std::string& name, const rapidjson::Value& v, std::string defaultVal) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return defaultVal;
    }
    assertIs<std::string>(name, m->value);
    return std::string(m->value.GetString());
  }

  /// \brief Get possible member with std::vector of values of template type T
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \param [in] defaultVal the value to be returned if the member is not found
  /// \return value of type std::vector of values of template type T
  /// \throws std::logic_error in case the member has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the type std::vector of values of template type T
  /// it returns the vector else the default one. If the member exists but has different type the exception std::logic_error is thrown.
  template<typename T>
  inline std::vector<T> getPossibleMemberAsVector(const std::string& name, const rapidjson::Value& v, std::vector<T> defaultVal = std::vector<T>())
  {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd())
      return defaultVal;

    const rapidjson::Value& vct = m->value;
    assertIsArray(name, vct);
    defaultVal.clear();

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      assertIs<T>(name, *itr);
      defaultVal.push_back(itr->Get<T>());
    }

    return defaultVal;
  }

  /// \brief Get member value if exists of template type T
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \param [out] member the value to be set if the member is found
  /// \return true if the member exists else false
  /// \throws std::logic_error in case the member exists but has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the type std::vector of values of type T
  /// it returns the vector else the default one. If the member exists but has different type the exception std::logic_error is thrown.
  template<typename T>
  inline bool getMemberIfExistsAs(const std::string& name, const rapidjson::Value& v, T& member) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return false;
    }
    assertIs<T>(name, m->value);
    member = m->value.Get<T>();
    return true;
  }

  /// \brief Get member value if exists of template type std::string
  /// \param [in] name json member
  /// \param [in] v json value to be examined for the member
  /// \param [out] member the value to be set if the member is found
  /// \return true if the member exists else false
  /// \throws std::logic_error in case the member exists but has different type
  /// \details
  /// Tries to find Json member named name and if it is found and holds a value of the type std::vector of values of type std::string
  /// it returns the vector else the default one. If the member exists but has different type the exception std::logic_error is thrown.
  /// It is template specialization for std::string.
  template<>
  inline bool getMemberIfExistsAs<std::string>(const std::string& name, const rapidjson::Value& v, std::string& member) {
    const auto m = v.FindMember(name.c_str());
    if (m == v.MemberEnd()) {
      return false;
    }
    assertIs<std::string>(name, m->value);
    member = m->value.GetString();
    return true;
  }

} //jutils
