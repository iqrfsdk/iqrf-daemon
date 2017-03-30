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

#include "PlatformDep.h"
#include "IqrfLogging.h"
#include <map>
#include <functional>
#include <memory>

template<typename T, typename R>
class ObjectFactory
{
private:
  typedef std::function<std::unique_ptr<T>(R&)> CreateObjectFunc;

  std::map<std::string, CreateObjectFunc> m_creators;

  template<typename S>
  static std::unique_ptr<T> createObject(R& representation) {
    return std::unique_ptr<T>(ant_new S(representation));
  }
public:

  template<typename S>
  void registerClass(const std::string& id){
    if (m_creators.find(id) != m_creators.end()){
      THROW_EX(std::logic_error, "Duplicit registration of: " << PAR(id));
    }
    m_creators.insert(std::make_pair(id, createObject<S>));
  }

  bool hasClass(const std::string& id){
    return m_creators.find(id) != m_creators.end();
  }

  std::unique_ptr<T> createObject(const std::string& id, R& representation){
    auto iter = m_creators.find(id);
    if (iter == m_creators.end()){
      THROW_EX(std::logic_error, "Unregistered parser for: " << PAR(id));
      //return std::unique_ptr<T>();
    }
    //calls the required createObject() function
    return std::move(iter->second(representation));
  }
};
