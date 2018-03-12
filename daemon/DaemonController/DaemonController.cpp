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

#include "PrfOs.h"
#include "DpaTransactionTask.h"

#include "UdpMessaging.h"
#include "IqrfLogging.h"
#include "LaunchUtils.h"
#include "DaemonController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"
#include "JsonUtils.h"

#include "IMessaging.h"
#include "UdpMessaging.h"
#include "Scheduler.h"

#include "PlatformDep.h"
#include "VersionInfo.h"

TRC_INIT()

using namespace rapidjson;

const char* MODE_OPERATIONAL("operational");
const char* MODE_SERVICE("service");
const char* MODE_FORWARDING("forwarding");

DaemonController& DaemonController::getController()
{
  static DaemonController mc;
  return mc;
}

void DaemonController::executeDpaTransaction(DpaTransaction& dpaTransaction)
{
  m_dpaTransactionQueue->pushToQueue(&dpaTransaction);
}

//called from task queue thread passed by lambda in task queue ctor
void DaemonController::executeDpaTransactionFunc(DpaTransaction* dpaTransaction)
{
  bool locked = m_modeMtx.try_lock();

  switch (m_mode) {

    //TODO lock mutex before change mode
  case Mode::Operational:
  {
    if (m_dpaHandler) {
      try {
        m_dpaHandler->ExecuteDpaTransaction(*dpaTransaction);
      }
      catch (std::exception& e) {
        CATCH_EX("Error in ExecuteDpaTransaction: ", std::exception, e);
        dpaTransaction->processFinish(DpaTransfer::kError);
      }
    }
    else {
      TRC_ERR("Dpa interface is not working");
      dpaTransaction->processFinish(DpaTransfer::kError);
    }
  }
  break;

  case Mode::Forwarding:
  {
    if (m_dpaHandler && m_dpaMessageForwarding) {
      auto dpaTransactionSniffer = m_dpaMessageForwarding->getDpaTransactionForward(dpaTransaction);
      try {
        m_dpaHandler->ExecuteDpaTransaction(*dpaTransactionSniffer);
      }
      catch (std::exception& e) {
        CATCH_EX("Error in ExecuteDpaTransaction: ", std::exception, e);
        dpaTransaction->processFinish(DpaTransfer::kError);
      }
    }
    else {
      TRC_ERR("Dpa interface is not working");
      dpaTransaction->processFinish(DpaTransfer::kError);
    }
  }
  break;

  case Mode::Service:
  {
    TRC_DBG("Dpa interface is in exclusiveMode");
    dpaTransaction->processFinish(DpaTransfer::kError);
  }
  break;

  default:;
  }

  if (locked)
    m_modeMtx.unlock();

  //Pet WatchDog
  watchDogPet();

}

void DaemonController::registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun)
{
  std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
  //TODO check success
  m_asyncMessageHandlers.insert(make_pair(serviceId, fun));

}

void DaemonController::unregisterAsyncMessageHandler(const std::string& serviceId)
{
  std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
  m_asyncMessageHandlers.erase(serviceId);
}

void DaemonController::asyncDpaMessageHandler(const DpaMessage& dpaMessage)
{
  std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
  for (auto & hndl : m_asyncMessageHandlers)
    hndl.second(dpaMessage);
}

DaemonController::DaemonController()
  :m_iqrfInterface(nullptr)
  , m_dpaHandler(nullptr)
  , m_dpaTransactionQueue(nullptr)
  , m_scheduler(nullptr)
  , m_modeStr(MODE_OPERATIONAL)
  , m_mode(Mode::Operational)
  , m_version(DAEMON_VERSION)
  , m_versionBuild(BUILD_TIMESTAMP)
{
}

void DaemonController::loadConfiguration(const std::string& cfgFileName)
{
  std::cout << "Loading configuration file: " << PAR(cfgFileName) << std::endl;
  m_cfgFileName = cfgFileName;

  jutils::parseJsonFile(m_cfgFileName, m_configuration);
  jutils::assertIsObject("", m_configuration);

  // check cfg version
  // TODO major.minor ...
  std::string cfgVersion = jutils::getMemberAs<std::string>("Configuration", m_configuration);
  if (cfgVersion != m_cfgVersion) {
    THROW_EX(std::logic_error, "Unexpected configuration: " << PAR(cfgVersion) << "expected: " << m_cfgVersion);
  }

  //prepare list
  m_configurationDir = jutils::getMemberAs<std::string>("ConfigurationDir", m_configuration);
  m_watchDogTimeoutMilis = jutils::getPossibleMemberAs<int>("WatchDogTimeoutMilis", m_configuration, m_watchDogTimeoutMilis);
  m_modeStr = jutils::getPossibleMemberAs<std::string>("Mode", m_configuration, m_modeStr);

  const auto m = jutils::getMember("Components", m_configuration);
  const rapidjson::Value& vct = m->value;
  jutils::assertIsArray("Components", vct);

  for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
    jutils::assertIsObject("Components[]", *itr);
    std::string componentName = jutils::getMemberAs<std::string>("ComponentName", *itr);
    bool enabled = jutils::getMemberAs<bool>("Enabled", *itr);
    auto inserted = m_componentMap.insert(make_pair(componentName, ComponentDescriptor(componentName, enabled)));
    inserted.first->second.loadConfiguration(m_configurationDir);
  }
}

DaemonController::~DaemonController()
{
}

void DaemonController::run(const std::string& cfgFileName)
{
  TRC_ENTER("");

  loadConfiguration(cfgFileName);

  m_timeout = std::chrono::milliseconds(m_watchDogTimeoutMilis);
  m_lastRefreshTime = std::chrono::system_clock::now();
  m_running = true;

  try {
    start();
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  {
    std::unique_lock<std::mutex> lock(m_stopConditionMutex);
    while (m_running && (m_watchDogTimeoutMilis == 0 || (std::chrono::system_clock::now() - m_lastRefreshTime) < m_timeout))
      m_stopConditionVariable.wait_for(lock, m_timeout);
  }

  bool wdog = m_running;

  if (wdog) {
    std::cout << std::endl << "!!!!!! WatchDog exit." << std::endl <<
      "Set appropriate <time> in millis or disable by set to zero in case of long DPA transactions" << std::endl <<
      "config.json: \"WatchDogTimeoutMilis\": <time>" << std::endl;
    TRC_ERR("Invoking WatchDog exit");
  }
  else {
    std::cout << std::endl << "..... Normal exit." << std::endl;
    TRC_ERR("Invoking Normal exit");
  }

  try {
    stop();
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  TRC_LEAVE("");
}

void DaemonController::watchDogPet()
{
  std::unique_lock<std::mutex> lock(m_stopConditionMutex);
  m_lastRefreshTime = std::chrono::system_clock::now();
}

void DaemonController::setMode(Mode mode)
{
  TRC_ENTER(NAME_PAR(mode, (int)mode));

  std::lock_guard<std::mutex> lck(m_modeMtx);

  switch (mode) {

  case Mode::Operational:
  {
    if (nullptr != m_dpaExclusiveAccess) {
      if (m_mode == Mode::Service) {
        m_dpaExclusiveAccess->resetExclusive();
        m_mode = mode;
        startDpa();
      }
    }
    TRC_INF("Set mode " << MODE_OPERATIONAL);
    m_mode = mode;
  }
  break;

  case Mode::Forwarding:
  {
    if (nullptr != m_dpaExclusiveAccess) {
      if (m_mode == Mode::Service) {
        m_dpaExclusiveAccess->resetExclusive();
        m_mode = mode;
        startDpa();
      }
      TRC_INF("Set mode " << MODE_FORWARDING);
      m_mode = mode;
    }
    else {
      TRC_INF("Cannot switch mode: forwarding component is not configured");
    }
  }
  break;

  case Mode::Service:
  {
    if (nullptr != m_dpaExclusiveAccess) {
      m_mode = mode;
      stopDpa();
      m_dpaExclusiveAccess->setExclusive(m_iqrfInterface);
      TRC_INF("Set mode " << MODE_SERVICE);
      m_mode = mode;
    }
    else {
      TRC_INF("Cannot switch mode: service component is not configured");
    }
  }
  break;

  default:;
  }
  TRC_LEAVE("");
}

///////// STARTS ////////////////////////////
void DaemonController::startTrace()
{
  auto fnd = m_componentMap.find("TracerFile");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      jutils::assertIsObject("", fnd->second.m_doc);

      m_traceFileName = jutils::getPossibleMemberAs<std::string>("TraceFileName", fnd->second.m_doc, "");
      m_traceFileSize = jutils::getPossibleMemberAs<int>("TraceFileSize", fnd->second.m_doc, 0);
      std::string vl = jutils::getPossibleMemberAs<std::string>("VerbosityLevel", fnd->second.m_doc, "dbg");

      if (vl == "err") m_level = iqrf::Level::err;
      else if (vl == "war") m_level = iqrf::Level::war;
      else if (vl == "inf") m_level = iqrf::Level::inf;
      else m_level = iqrf::Level::dbg;

      if (m_traceFileSize <= 0)
        m_traceFileSize = TRC_DEFAULT_FILE_MAXSIZE;

      TRC_START(m_traceFileName, m_level, m_traceFileSize);
      TRC_INF(std::endl <<
        "============================================================================" << std::endl <<
        PAR(DAEMON_VERSION) << PAR(BUILD_TIMESTAMP) << std::endl <<
        "============================================================================" << std::endl
      )
        TRC_INF("Loaded configuration file: " << PAR(m_cfgFileName));
      TRC_INF("Opened trace file: " << PAR(m_traceFileName) << PAR(m_traceFileSize));

    }
    catch (std::exception &e) {
      std::cout << "Cannot create TracerFile: " << e.what() << std::endl;
    }
  }
}

void DaemonController::startIqrfIf()
{
  spi_iqrf_config_struct cfg(IqrfSpiChannel::SPI_IQRF_CFG_DEFAULT);

  auto fnd = m_componentMap.find("IqrfInterface");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      jutils::assertIsObject("", fnd->second.m_doc);
      m_iqrfInterfaceName = jutils::getMemberAs<std::string>("IqrfInterface", fnd->second.m_doc);
      
      memset(cfg.spiDev, 0, sizeof(cfg.spiDev));
      auto sz = m_iqrfInterfaceName.size();
      if (sz > sizeof(cfg.spiDev)) sz = sizeof(cfg.spiDev);
      std::copy(m_iqrfInterfaceName.c_str(), m_iqrfInterfaceName.c_str() + sz, cfg.spiDev);
      
      cfg.enableGpioPin = jutils::getPossibleMemberAs<int>("enableGpioPin", fnd->second.m_doc, cfg.enableGpioPin);
      cfg.spiCe0GpioPin = jutils::getPossibleMemberAs<int>("spiCe0GpioPin", fnd->second.m_doc, cfg.spiCe0GpioPin);
      cfg.spiMisoGpioPin = jutils::getPossibleMemberAs<int>("spiMisoGpioPin", fnd->second.m_doc, cfg.spiMisoGpioPin);
      cfg.spiMosiGpioPin = jutils::getPossibleMemberAs<int>("spiMosiGpioPin", fnd->second.m_doc, cfg.spiMosiGpioPin);
      cfg.spiClkGpioPin = jutils::getPossibleMemberAs<int>("spiClkGpioPin", fnd->second.m_doc, cfg.spiClkGpioPin);

      m_dpaHandlerTimeout = jutils::getPossibleMemberAs<int>("DpaHandlerTimeout", fnd->second.m_doc, m_dpaHandlerTimeout);

      std::string communicationMode;
      communicationMode = jutils::getPossibleMemberAs<std::string>("CommunicationMode", fnd->second.m_doc, communicationMode);
      if (communicationMode == "LP")
        m_communicationMode = IqrfRfCommunicationMode::kLp;
      else if (communicationMode == "STD")
        m_communicationMode = IqrfRfCommunicationMode::kStd;
      else
        m_communicationMode = IqrfRfCommunicationMode::kStd;


      TRC_INF(PAR(m_iqrfInterfaceName));

      int attempts = 1;
      while (attempts < 3) {
        try {
          size_t found = m_iqrfInterfaceName.find("spi");
          
          if (found != std::string::npos)
            m_iqrfInterface = ant_new IqrfSpiChannel(cfg);
          else
            m_iqrfInterface = ant_new IqrfCdcChannel(m_iqrfInterfaceName);

          break;
        }
        catch (std::exception &e) {
          CATCH_EX(PAR(attempts) << " to create IqrfInterface failure: ", std::exception, e);
          ++attempts;
          std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
      }

    }
    catch (std::exception &e) {
      CATCH_EX("Cannot create IqrfInterface: ", std::exception, e);
    }
  }

  // wait for iqrfInterface ready
  if (nullptr != m_iqrfInterface) {
    int att = 10;
    IChannel::State st = m_iqrfInterface->getState();

    while (IChannel::State::Ready != st)
    {
      watchDogPet();

      if (--att >= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      else {
        TRC_ERR("IQRF interface is not ready ...");
        break;
      }

      st = m_iqrfInterface->getState();
    }
  }

  m_dpaTransactionQueue = ant_new TaskQueue<DpaTransaction*>([&](DpaTransaction* trans) {
    executeDpaTransactionFunc(trans);
  });
}

void DaemonController::startDpa()
{
  try {
    m_dpaHandler = ant_new DpaHandler(m_iqrfInterface);
    if (m_dpaHandlerTimeout > 0) {
      m_dpaHandler->Timeout(m_dpaHandlerTimeout);
    }
    else {
      // 400ms by default
      m_dpaHandler->Timeout(DpaHandler::DEFAULT_TIMING);
    }
    
    m_dpaHandler->SetRfCommunicationMode(m_communicationMode);

    //Async msg handling
    m_dpaHandler->RegisterAsyncMessageHandler([&](const DpaMessage& dpaMessage) {
      asyncDpaMessageHandler(dpaMessage);
    });

    //TR module
    PrfOs prfOs;
    prfOs.read();
    
    DpaTransactionTask trans(prfOs);
    executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (result != 0) {
      THROW_EX(std::logic_error, "Cannot get TR parameters");
    }

    m_moduleId = prfOs.getModuleId();
    m_osVersion = prfOs.getOsVersion();
    m_trType = prfOs.getTrType();
    m_mcuType = prfOs.getMcuType();
    m_osBuild = prfOs.getOsBuild();

  }


  catch (std::exception& ae) {
    TRC_ERR("There was an error during DPA handler creation: " << ae.what());
  }
}

void DaemonController::startScheduler()
{
  Scheduler* schd = ant_new Scheduler();
  m_scheduler = schd;

  auto fnd = m_componentMap.find("Scheduler");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      jutils::assertIsObject("", fnd->second.m_doc);
      schd->updateConfiguration(fnd->second.m_doc);
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot create Scheduler: ", std::exception, e);
    }
  }

  //m_scheduler->start();

  //Pet WatchDog at least from Scheduler if there is no DpaTransaction
  m_scheduler->registerMessageHandler("WatchDogPet", [this](const std::string& msg) {
    watchDogPet();
  });
  m_scheduler->scheduleTaskPeriodic("WatchDogPet", "WatchDogPet",
    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(m_watchDogTimeoutMilis / 2)));

}

void DaemonController::loadSerializerComponent(const ComponentDescriptor& componentDescriptor)
{
  if (componentDescriptor.m_interfaceName != "ISerializer")
    return;

  typedef std::unique_ptr<ISerializer>(*CreateSerializer)(const std::string&);

  const auto instancesMember = jutils::getMember("Instances", componentDescriptor.m_doc);
  const rapidjson::Value& instances = instancesMember->value;
  jutils::assertIsArray("Instances[]", instances);

  for (auto itr = instances.Begin(); itr != instances.End(); ++itr) {
    //parse instance descriptor
    jutils::assertIsObject("Instances[]{}", *itr);
    std::string instanceName = jutils::getMemberAs<std::string>("Name", *itr);
    bool enabled = jutils::getPossibleMemberAs<bool>("Enabled", *itr, true);
    if (!enabled)
      continue;

    const auto propertiesMember = jutils::getMember("Properties", *itr);
    const rapidjson::Value& properties = propertiesMember->value;
    jutils::assertIsObject("Properties{}", properties);

    //create instance
    CreateSerializer createSerializer = (CreateSerializer)getCreateFunction(componentDescriptor.m_componentName, true);
    std::unique_ptr<ISerializer> serializer = createSerializer(instanceName);

    //get properties - not implemented
    //serializer->update(properties);

    //register instance
    auto ret = m_serializers.insert(std::make_pair(serializer->getName(), std::move(serializer)));
    if (!ret.second) {
      THROW_EX(std::logic_error, "Duplicit Serializer configuration: " << NAME_PAR(name, ret.first->first));
    }
  }
}

void DaemonController::loadMessagingComponent(const ComponentDescriptor& componentDescriptor)
{
  if (componentDescriptor.m_interfaceName != "IMessaging")
    return;

  typedef std::unique_ptr<IMessaging>(*CreateMessaging)(const std::string&);

  const auto instancesMember = jutils::getMember("Instances", componentDescriptor.m_doc);
  const rapidjson::Value& instances = instancesMember->value;
  jutils::assertIsArray("Instances[]", instances);

  for (auto itr = instances.Begin(); itr != instances.End(); ++itr) {
    //parse instance descriptor
    jutils::assertIsObject("Instances[]{}", *itr);
    std::string instanceName = jutils::getMemberAs<std::string>("Name", *itr);
    bool enabled = jutils::getPossibleMemberAs<bool>("Enabled", *itr, true);
    if (!enabled)
      continue;

    const auto propertiesMember = jutils::getMember("Properties", *itr);
    const rapidjson::Value& properties = propertiesMember->value;
    jutils::assertIsObject("Properties{}", properties);

    //create instance
    CreateMessaging createMessaging = (CreateMessaging)getCreateFunction(componentDescriptor.m_componentName, true);
    std::unique_ptr<IMessaging> messaging = createMessaging(instanceName);

    //get properties
    messaging->update(properties);

    //temporary hack to get other ifaces implemented by UdpMessaging
    if (componentDescriptor.m_componentName == "UdpMessaging") {
      UdpMessaging* udpMessaging = dynamic_cast<UdpMessaging*>(messaging.get());
      if (udpMessaging != nullptr) {
        m_dpaExclusiveAccess = udpMessaging;
        m_dpaMessageForwarding = udpMessaging;
        udpMessaging->setDaemon(this);
      }
    }

    //register instance
    auto ret = m_messagings.insert(std::make_pair(messaging->getName(), std::move(messaging)));
    if (!ret.second) {
      THROW_EX(std::logic_error, "Duplicit Messaging configuration: " << NAME_PAR(name, ret.first->first));
    }
  }
}

void DaemonController::loadServiceComponent(const ComponentDescriptor& componentDescriptor)
{
  if (componentDescriptor.m_interfaceName != "IService")
    return;

  typedef std::unique_ptr<IService>(*CreateBaseService)(const std::string&);

  const auto instancesMember = jutils::getMember("Instances", componentDescriptor.m_doc);
  const rapidjson::Value& instances = instancesMember->value;
  jutils::assertIsArray("Instances[]", instances);

  for (auto itr = instances.Begin(); itr != instances.End(); ++itr) {
    try {
      //parse instance descriptor
      jutils::assertIsObject("Instances[]{}", *itr);
      std::string instanceName = jutils::getMemberAs<std::string>("Name", *itr);
      bool enabled = jutils::getPossibleMemberAs<bool>("Enabled", *itr, true);
      if (!enabled)
        continue;

      TRC_INF("Creating: " << PAR(instanceName));
      std::string messagingName = jutils::getMemberAs<std::string>("Messaging", *itr);
      std::vector<std::string> serializersVector = jutils::getMemberAsVector<std::string>("Serializers", *itr);
      const auto propertiesMember = jutils::getMember("Properties", *itr);
      const rapidjson::Value& properties = propertiesMember->value;
      jutils::assertIsObject("Properties{}", properties);

      //create instance
      CreateBaseService createService = (CreateBaseService)getCreateFunction(componentDescriptor.m_componentName, true);
      std::unique_ptr<IService> service = createService(instanceName);
      service->setDaemon(this);

      //get messaging
      {
        auto found = m_messagings.find(messagingName);
        if (found != m_messagings.end()) {
          service->setMessaging(found->second.get());
        }
        else
          THROW_EX(std::logic_error, "Cannot find: " << PAR(messagingName));
      }

      //get serializers
      for (auto & serializerName : serializersVector) {
        auto ss = m_serializers.find(serializerName);
        if (ss != m_serializers.end())
          service->setSerializer(ss->second.get());
        else
          THROW_EX(std::logic_error, "Cannot find: " << PAR(serializerName));
      }

      //get properties
      service->update(properties);

      //register instance
      auto ret = m_services.insert(std::make_pair(service->getName(), std::move(service)));
      if (!ret.second) {
        THROW_EX(std::logic_error, "Duplicit Service configuration: " << NAME_PAR(name, ret.first->first));
      }
    }
    catch (std::logic_error &e) {
      CATCH_EX("Cannot create component instance: ", std::exception, e);
    }
  }
}

void DaemonController::startServices()
{
  TRC_ENTER("");

  //load serializer components
  for (const auto & mc : m_componentMap) {
    if (mc.second.m_enabled) {
      try {
        loadSerializerComponent(mc.second);
      }
      catch (std::exception &e) {
        CATCH_EX("Cannot create component" << NAME_PAR(componentName, mc.first), std::exception, e);
      }
    }
  }

  //load messaging components
  for (const auto & mc : m_componentMap) {
    if (mc.second.m_enabled) {
      try {
        loadMessagingComponent(mc.second);
      }
      catch (std::exception &e) {
        CATCH_EX("Cannot create component" << NAME_PAR(componentName, mc.first), std::exception, e);
      }
    }
  }

  //load service components
  for (const auto & mc : m_componentMap) {
    if (mc.second.m_enabled) {
      try {
        loadServiceComponent(mc.second);
      }
      catch (std::exception &e) {
        CATCH_EX("Cannot create component" << NAME_PAR(componentName, mc.first), std::exception, e);
      }
    }
  }

  //start services
  for (auto & cli : m_services) {
    try {
      cli.second->start();
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot start " << NAME_PAR(service, cli.second->getName()), std::exception, e);
    }
  }

  //start messagings
  for (auto & ms : m_messagings) {
    try {
      ms.second->start();
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot start messaging ", std::exception, e);
    }
  }

  //all is ready => start Scheduler at the end
  m_scheduler->start();

  TRC_LEAVE("");
}

void DaemonController::start()
{
  TRC_ENTER("");

  startTrace();
  startIqrfIf();
  startDpa();
  startScheduler();
  startServices();

  if (MODE_OPERATIONAL != m_modeStr) {
    doCommand(m_modeStr);
  }

  TRC_INF("daemon started");

  std::cout << "IQRF Gateway Daemon started ....." << std::endl;

  TRC_LEAVE("");
}

///////// STOPS ////////////////////////////

void DaemonController::stopTrace()
{
}

void DaemonController::stopIqrfIf()
{
  TRC_ENTER("");
  TRC_DBG("Try to destroy: " << PAR(m_dpaTransactionQueue->size()));
  delete m_dpaTransactionQueue;
  m_dpaTransactionQueue = nullptr;

  TRC_DBG("Try to destroy: " << PAR(m_iqrfInterface));
  delete m_iqrfInterface;
  m_iqrfInterface = nullptr;
  TRC_LEAVE("");
}

void DaemonController::stopDpa()
{
  TRC_ENTER("");
  TRC_DBG("Try to destroy: " << PAR(m_dpaHandler));
  delete m_dpaHandler;
  m_dpaHandler = nullptr;
  TRC_LEAVE("");
}

void DaemonController::stopServices()
{
  TRC_ENTER("");
  TRC_DBG("Stopping: " << PAR(m_services.size()));
  for (auto & cli : m_services) {
    cli.second->stop();
  }
  m_services.clear();

  TRC_DBG("Stopping: " << PAR(m_messagings.size()));
  for (auto & ms : m_messagings) {
    ms.second->stop();
  }
  m_messagings.clear();

  for (auto & sr : m_serializers) {
  }
  m_serializers.clear();

  TRC_LEAVE("");
}

void DaemonController::stop()
{
  TRC_ENTER("");
  TRC_INF("daemon stops");

  TRC_DBG("Stopping: " << PAR(m_scheduler));
  m_scheduler->stop();

  TRC_DBG("Stopping: " << PAR(m_dpaTransactionQueue->size()));
  m_dpaTransactionQueue->stopQueue();

  if (nullptr != m_dpaHandler) {
    TRC_DBG("Killing DpaTransaction if any");
    m_dpaHandler->KillDpaTransaction();
  }

  stopServices();
  TRC_DBG("daemon: before stopDpa");
  stopDpa();
  stopIqrfIf();

  TRC_DBG("Try to destroy: " << PAR(m_scheduler));
  delete m_scheduler;
  TRC_DBG("daemon: after delete m_scheduler");

  stopTrace();

  TRC_LEAVE("");
}

void DaemonController::exit()
{
  TRC_INF("exiting ...");
  {
    std::unique_lock<std::mutex> lock(m_stopConditionMutex);
    m_running = false;
  }
  m_stopConditionVariable.notify_all();
}

std::string DaemonController::doCommand(const std::string& cmd)
{
  std::string res = "ERROR_UNKNOWN";
  if (m_iqrfInterface != nullptr) {
    if (cmd == MODE_OPERATIONAL) {
      setMode(Mode::Operational);
      res = "OK";
    }
    if (cmd == MODE_SERVICE) {
      setMode(Mode::Service);
      res = "OK";
    }
    if (cmd == MODE_FORWARDING) {
      setMode(Mode::Forwarding);
      res = "OK";
    }
  }
  else {
    res = "ERROR_IFACE";
  }
  return res;
}

//////////////// Launch
void * DaemonController::getCreateFunction(const std::string& componentName, bool mandatory) const
{
  std::string fname = ("__launch_create_");
  fname += componentName;
  return getFunction(fname, mandatory);
}

void * DaemonController::getFunction(const std::string& methodName, bool mandatory) const
{
  void * func = nullptr;

  func = StaticBuildFunctionMap::get().getFunction(methodName);
  if (!func) {
    if (mandatory) {
      THROW_EX(std::logic_error, PAR(methodName) << ": cannot be found. The binary isn't probably correctly linked");
    }
    else {
      TRC_WAR(PAR(methodName) << ": cannot be found. The binary isn't probably correctly linked");
    }
  }
  return func;
}

///////////////////////////////////// ComponentDescriptor
void ComponentDescriptor::loadConfiguration(const std::string configurationDir)
{
  std::ostringstream os;
  os << configurationDir << '/' << m_componentName << ".json";

  try {
    jutils::parseJsonFile(os.str(), m_doc);
    m_interfaceName = jutils::getPossibleMemberAs<std::string>("Implements", m_doc, "");
  }
  catch (std::exception &e) {
    CATCH_EX("Cannot load file " << NAME_PAR(fname, os.str()), std::exception, e);
    std::cerr << std::endl << "Cannot load file " << NAME_PAR(fname, os.str()) << NAME_PAR(error, e.what()) << std::endl;
  }

}
