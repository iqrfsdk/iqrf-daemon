#pragma once

#include "PlatformDep.h"
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
      //TODO error handling
    }
    m_creators.insert(std::make_pair(id, createObject<S>));
  }

  bool hasClass(const std::string& id){
    return m_creators.find(id) != m_creators.end();
  }

  std::unique_ptr<T> createObject(const std::string& id, R& representation){
    auto iter = m_creators.find(id);
    if (iter == m_creators.end()){
      return std::unique_ptr<T>();
    }
    //calls the required createObject() function
    return std::move(iter->second(representation));
  }
};
