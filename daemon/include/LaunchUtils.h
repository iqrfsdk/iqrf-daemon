#pragma once

#include "IqrfLogging.h"
#include <memory>
#include <string>
#include <map>

class StaticBuildFunctionMap
{
public:
  static StaticBuildFunctionMap& get()
  {
    static StaticBuildFunctionMap s;
    return s;
  }
  virtual ~StaticBuildFunctionMap() {}

  void* getFunction(const std::string& fceName) const
  {
    auto found = m_fceMap.find(fceName);
    if (found != m_fceMap.end()) {
      return found->second;
    }
    return nullptr;
  }

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

//#define CREATE_CLIENT_SERVICE(clientServiceClassName)                                         \
//std::shared_ptr<IClient> __launch_create_##clientServiceClassName(const std::string& iname) { \
//   return std::shared_ptr<IClient>(new clientServiceClassName(iname)); }                      \
//                                                                                              \
//class __Loader_##clientServiceClassName {                                                     \
//public:                                                                                       \
//  __Loader_##clientServiceClassName() {                                                       \
//      StaticBuildFunctionMap::get().setFunction("__launch_create_" #clientServiceClassName,   \
//        (void*)&__launch_create_##clientServiceClassName);                                    \
//  }                                                                                           \
//};                                                                                            \
//                                                                                              \
//void init_##clientServiceClassName() {                                                        \
//  static __Loader_##clientServiceClassName cs;                                                \
//}
