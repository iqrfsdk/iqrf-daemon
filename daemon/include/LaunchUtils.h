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
#include <memory>
#include <string>
#include <map>

/// \class StaticBuildFunctionMap
/// \brief Holds pointers to component loader functions
class StaticBuildFunctionMap
{
public:
  /// \brief StaticBuildFunctionMap singleton getter
  static StaticBuildFunctionMap& get()
  {
    static StaticBuildFunctionMap s;
    return s;
  }

  virtual ~StaticBuildFunctionMap() {}

  /// \brief Get pointer to function
  /// \param [in] fceName function name
  /// \return pointer to function if exists else nullptr
  /// \details
  /// Searches the map and returns pointer to function with passed nameFile. If the name doesn't
  /// exist it return nullptr
  void* getFunction(const std::string& fceName) const
  {
    auto found = m_fceMap.find(fceName);
    if (found != m_fceMap.end()) {
      return found->second;
    }
    return nullptr;
  }

  /// \brief Set pointer to function
  /// \param [in] fceName function name
  /// \param [in] fcePtr function pointer
  /// \throws std::logic_error in case the name is already stored
  /// \details
  /// Insert pair [name, pointer] to the map. If the map already contains name it throws std::logic_error
  void setFunction(const std::string& fceName, void* fcePtr)
  {
    auto inserted = m_fceMap.insert(make_pair(fceName, fcePtr));
    if (!inserted.second) {
      std::cerr << PAR(fceName) << ": is already stored. The binary isn't probably correctly linked" << std::endl;
      THROW_EX(std::logic_error, PAR(fceName) << ": is already stored. The binary isn't probably correctly linked");
    }
  }

private:
  StaticBuildFunctionMap() {}
  std::map<std::string, void*> m_fceMap;
};

/// \brief Convenient macro to register component interface and implementation
/// \param IComponentClassName component interface name
/// \param ComponentClassName component implementation name
/// \details
/// Declare and define
/// define __launch_create_##ComponentClassName() to create implementation
/// declare __Loader_##ComponentClassName class for function registration
/// define init_##ComponentClassName() with static class for function registration
/// This macro shall be added once to a component source code and init_##ComponentClassName()
/// shall be called at the very beginning of application in case of static build
#define INIT_COMPONENT(IComponentClassName, ComponentClassName)                                         \
std::unique_ptr<IComponentClassName> __launch_create_##ComponentClassName(const std::string& iname) {   \
   return std::unique_ptr<IComponentClassName>(new ComponentClassName(iname)); }                      \
                                                                                              \
class __Loader_##ComponentClassName {                                                     \
public:                                                                                       \
  __Loader_##ComponentClassName() {                                                       \
      StaticBuildFunctionMap::get().setFunction("__launch_create_" #ComponentClassName,   \
        (void*)&__launch_create_##ComponentClassName);                                    \
  }                                                                                           \
};                                                                                            \
                                                                                              \
void init_##ComponentClassName() {                                                        \
  static __Loader_##ComponentClassName cs;                                                \
}
