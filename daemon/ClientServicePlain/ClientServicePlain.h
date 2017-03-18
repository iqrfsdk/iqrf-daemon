#pragma once

#include "JsonUtils.h"
#include "IClient.h"
#include "ISerializer.h"
#include "IMessaging.h"
#include "IScheduler.h"
#include <string>
#include <vector>

class IDaemon;

typedef std::basic_string<unsigned char> ustring;

class ClientServicePlain : public IClient
{
public:
  ClientServicePlain() = delete;
  ClientServicePlain(const std::string& name);
  virtual ~ClientServicePlain();

  void setDaemon(IDaemon* daemon) override;
  virtual void setSerializer(ISerializer* serializer) override;
  virtual void setMessaging(IMessaging* messaging) override;
  const std::string& getClientName() const override {
    return m_name;
  }
  
  void update(const rapidjson::Value& cfg) override;
  void start() override;
  void stop() override;

private:
  void handleMsgFromMessaging(const ustring& msg);

  std::string m_name;

  IMessaging* m_messaging;
  IDaemon* m_daemon;
  std::vector<ISerializer*> m_serializerVect;
};
