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

#include "DpaHandler.h"
#include "ISerializer.h"
#include "IMessaging.h"
#include "IService.h"
#include "TaskQueue.h"
#include "IDaemon.h"
#include "JsonUtils.h"
#include "IqrfLogging.h"
#include <map>
#include <string>
#include <atomic>
#include <vector>

class IChannel;
class IDpaMessageForwarding;
class IDpaExclusiveAccess;

/// \class ComponentDescriptor
/// \brief Auxiliar class for component initialization
/// \details
/// It is used by DaemonController to split and store components configuration from configuration file passed by cmdl
class ComponentDescriptor {
public:
  
  /// \brief parametric constructor
  /// \param [in] componentName component name
  /// \param [in] enabled component is enabled
  /// \details
  /// Configuration data for particular component
  ComponentDescriptor(const std::string& componentName, bool enabled)
    : m_componentName(componentName)
    , m_enabled(enabled)
  {}

  /// \brief load JSON configuration file
  /// \param [in] configurationDir directory with component configuration file
  /// \details
  /// Load and parse JSON configuration file for component.
  void loadConfiguration(const std::string configurationDir);

  /// \brief get JSON configuration
  /// \return JSON configuration data
  /// \details
  /// Returns JSON configuration data obtained from component configuration file
  rapidjson::Value& getConfiguration() {
    return m_doc;
  }

  std::string m_componentName;
  std::string m_interfaceName;
  bool m_enabled = false;
  rapidjson::Document m_doc;
};

/// \class DaemonController
/// \brief Create component instances
/// \details
/// Based on configuration file passed from cmdl it instantiate and bind all component instances.
/// The instances together implements configured daemon functions.
/// As this version of daemon is statically linked, the components must be linked with iqrf_startup executable.
/// It starts watch dog to monitor comunication threads and invoke exit in case of something got stucked
///
/// It accepts configuration JSON file:
/// ```json
/// {
///   "Configuration": "v0.0",                #configuration version
///   "ConfigurationDir" : "configuration",   #configuration directory
///   "WatchDogTimeoutMilis" : 10000,         #watch dog timeout
///   "Mode" : "operational",                 #operational mode: operational | forwarding | service
///   "Components" : [                        #components to be instantiated
///     {
///       "ComponentName": "BaseService",     #component name
///       "Enabled" : true                    #if true component is instantiated else ignored
///     },
///     ...
///   ]
/// }
/// ```
class DaemonController : public IDaemon
{
public:
  /// \brief operational mode
  /// \details
  /// Operational is used for normal work
  /// Service the only UDP Messaging is used to communicate with IQRF IDE
  /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
  enum class Mode {
    Operational,
    Service,
    Forwarding
  };

  DaemonController(const DaemonController &) = delete;

  static DaemonController& getController();
  
  /// main loop
  void run(const std::string& cfgFileName);

  // IDaemon override methods
  void executeDpaTransaction(DpaTransaction& dpaTransaction) override;
  void registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) override;
  void unregisterAsyncMessageHandler(const std::string& serviceId) override;
  IScheduler* getScheduler() override { return m_scheduler; }
  std::string doCommand(const std::string& cmd) override;
  const std::string& getModuleId() override { return m_moduleId; }
  const std::string& getOsVersion() override { return m_osVersion; }
  const std::string& getTrType() override { return m_trType; }
  const std::string& getMcuType() override { return m_mcuType; }
  const std::string& getOsBuild() override { return m_osBuild; }
  const std::string& getDaemonVersion() override { return m_version; }
  const std::string& getDaemonVersionBuild() override { return m_versionBuild; }

  /// \brief Invoke exit of this process
  void exit();

  /// \brief switch operational mode
  /// \param [in] mode operational mode to switch
  /// \details
  /// \details
  /// Operational is used for normal work
  /// Service the only UDP Messaging is used to communicate with IQRF IDE
  /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
  void setMode(Mode mode);

private:
  std::mutex m_modeMtx;
  Mode m_mode;

  std::map<std::string, AsyncMessageHandlerFunc> m_asyncMessageHandlers;
  std::mutex m_asyncMessageHandlersMutex;
  void asyncDpaMessageHandler(const DpaMessage& dpaMessage);

  DaemonController();
  virtual ~DaemonController();

  void startTrace();
  void startIqrfIf();
  void startDpa();
  void startServices();
  void startScheduler();

  void stopIqrfIf();
  void stopDpa();
  void stopServices();
  void stopTrace();

  void start();
  void stop();

  void * getFunction(const std::string& methodName, bool mandatory) const;
  void * getCreateFunction(const std::string& componentName, bool mandatory) const;

  IChannel* m_iqrfInterface;
  DpaHandler* m_dpaHandler;
  
  void executeDpaTransactionFunc(DpaTransaction* dpaTransaction);

  TaskQueue<DpaTransaction*> *m_dpaTransactionQueue;

  std::map<std::string, std::unique_ptr<ISerializer>> m_serializers;
  std::map<std::string, std::unique_ptr<IService>> m_services;
  std::map<std::string, std::unique_ptr<IMessaging>> m_messagings;

  IDpaMessageForwarding* m_dpaMessageForwarding = nullptr;
  IDpaExclusiveAccess* m_dpaExclusiveAccess = nullptr;

  IScheduler* m_scheduler = nullptr;

  /// watchDog
  void watchDogPet();
  bool m_running = false;
  std::mutex m_stopConditionMutex;
  std::condition_variable m_stopConditionVariable;
  std::chrono::system_clock::time_point m_lastRefreshTime;
  std::chrono::milliseconds m_timeout;

  /// configuration
  void loadConfiguration(const std::string& cfgFileName);
  rapidjson::Document m_configuration;
  std::string m_cfgFileName;
  const std::string m_cfgVersion = "v1.0";
  std::string m_traceFileName;
  int m_traceFileSize = 0;
  iqrf::Level m_level;
  std::string m_iqrfInterfaceName;
  int m_dpaHandlerTimeout = 400;
  IqrfRfCommunicationMode m_communicationMode = IqrfRfCommunicationMode::kStd;

  std::string m_configurationDir;
  std::string m_modeStr;
  int m_watchDogTimeoutMilis = 0;
  std::map<std::string, ComponentDescriptor> m_componentMap;

  void loadSerializerComponent(const ComponentDescriptor& componentDescriptor);
  void loadMessagingComponent(const ComponentDescriptor& componentDescriptor);
  void loadServiceComponent(const ComponentDescriptor& componentDescriptor);

  /// TR module
  std::string m_moduleId;
  std::string m_osVersion;
  std::string m_trType;
  bool m_fcc = false;
  std::string m_mcuType;
  std::string m_osBuild;

  std::string m_version;
  std::string m_versionBuild;
};
