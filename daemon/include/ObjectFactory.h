#pragma once

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
    return std::unique_ptr<T>(new S(representation));
  }
public:

  template<typename S>
  void registerClass(const std::string& id){
    if (m_creators.find(id) != m_creators.end()){
      //your error handling here
    }
    m_creators.insert(std::make_pair(id, createObject<S>));
  }

  bool hasClass(const std::string& id){
    return mObjectCreator.find(id) != mObjectCreator.end();
  }

  std::unique_ptr<T> createObject(const std::string& id, R& representation){
    //Don't use hasClass here as doing so would involve two lookups
    auto iter = m_creators.find(id);
    if (iter == m_creators.end()){
      return std::unique_ptr<T>();
    }
    //calls the required createObject() function
    return std::move(iter->second(representation));
  }
};

#if 0
/**
* A class for creating objects, with the type of object created based on a key
*
* @param K the key
* @param T the super class that all created classes derive from
*/
template<typename K, typename T>
class ObjectFactory {
private:
  typedef std::function<std::unique_ptr<T>()> CreateObjectFunc;

  /**
  * A map keys (K) to functions (CreateObjectFunc)
  * When creating a new type, we simply call the function with the required key
  */
  std::map<K, CreateObjectFunc> m_creators;

  /**
  * Pointers to this function are inserted into the map and called when creating objects
  *
  * @param S the type of class to create
  * @return a object with the type of S
  */
  template<typename S>
  static std::unique_ptr<T> createObject() {
    return std::unique_ptr<T>(new S());
  }
public:

  /**
  * Registers a class to that it can be created via createObject()
  *
  * @param S the class to register, this must ve a subclass of T
  * @param id the id to associate with the class. This ID must be unique
  */
  template<typename S>
  void registerClass(const K& id){
    if (m_creators.find(id) != m_creators.end()){
      //your error handling here
    }
    m_creators.insert(std::make_pair(id, createObject<S>));
  }

  /**
  * Returns true if a given key exists
  *
  * @param id the id to check exists
  * @return true if the id exists
  */
  bool hasClass(const K& id){
    return mObjectCreator.find(id) != mObjectCreator.end();
  }

  /**
  * Creates an object based on an id. It will return null if the key doesn't exist
  *
  * @param id the id of the object to create
  * @return the new object or null if the object id doesn't exist
  */
  std::unique_ptr<T> createObject(K id){
    //Don't use hasClass here as doing so would involve two lookups
    auto iter = m_creators.find(id);
    if (iter == m_creators.end()){
      return std::unique_ptr<T>();
    }
    //calls the required createObject() function
    return std::move(iter->second());
  }
};
#endif
