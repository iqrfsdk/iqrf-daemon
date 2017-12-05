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

#include "PlatformDep.h"
#include "IqrfLogging.h"
#include <map>
#include <functional>
#include <memory>

/// \class ObjectFactory
/// \brief Create object based on data representation
/// \details
/// Allows registering object creator functions for classes created from their representations.
/// Typically used for parsing incomming messages. The messages are preparsed to get type of class
/// to be parsed and the type is passed together with the message to createObject method. The method
/// creates an object via matching object creator and message data representation
template<typename T, typename R>
class ObjectFactory
{
private:
  /// Object creator function type
  typedef std::function<std::unique_ptr<T>(R&)> CreateObjectFunc;
  /// Map of object creators
  std::map<std::string, CreateObjectFunc> m_creators;

  /// \brief Template function to create object of type S
  /// \param [in] representation object to be used for creation
  /// \return created object
  /// \details
  /// Create object of type T based on data held in representation R.
  /// For example type R can be a class representing JSON message to be used to create class T
  /// described by the message
  template<typename S>
  static std::unique_ptr<T> createObject(R& representation) {
    return std::unique_ptr<T>(ant_new S(representation));
  }
public:
  /// \brief Template function to register object creator function
  /// \param [in] name of class to be registered
  /// \throws std::logic_error in case of duplicity
  /// \details
  /// Stores to registrar map pair id and object creator function for class type S. In case id is already used
  /// the exception std::logic_error is thrown
  template<typename S>
  void registerClass(const std::string& id){
    if (m_creators.find(id) != m_creators.end()){
      THROW_EX(std::logic_error, "Duplicit registration of: " << PAR(id));
    }
    m_creators.insert(std::make_pair(id, createObject<S>));
  }

  /// \brief Check if a creator object is registered
  /// \param [in] id is name representing the class
  /// \return true if registered else false
  /// \details
  /// Check registrar map if object creator for class named id is registered if yes return true else false
  bool hasClass(const std::string& id){
    return m_creators.find(id) != m_creators.end();
  }

  /// \brief Create object based on data representation
  /// \details
  /// \param [in] id is name representing the class
  /// \param [in] representation data representing object to be created
  /// \return created object
  /// \throws std::logic_error in case of creator is not registered for passed id
  /// Crreates an object via matching object creator and message data representation
  std::unique_ptr<T> createObject(const std::string& id, R& representation){
    auto iter = m_creators.find(id);
    if (iter == m_creators.end()){
      THROW_EX(std::logic_error, "Unregistered creator for: " << PAR(id));
    }
    //calls the required createObject() function
    return std::move(iter->second(representation));
  }
};
