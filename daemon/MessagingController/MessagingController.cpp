#include "IqrfLogging.h"
#include "LaunchUtils.h"
#include "MessagingController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"
#include "JsonUtils.h"

#include "IClient.h"

#include "IMessaging.h"
#include "UdpMessaging.h"
#include "Scheduler.h"

#include "PlatformDep.h"

TRC_INIT()

using namespace rapidjson;

MessagingController& MessagingController::getController()
{
  static MessagingController mc;
  return mc;
}

void MessagingController::executeDpaTransaction(DpaTransaction& dpaTransaction)
{
  m_dpaTransactionQueue->pushToQueue(&dpaTransaction);
}

//called from task queue thread passed by lambda in task queue ctor
void MessagingController::executeDpaTransactionFunc(DpaTransaction* dpaTransaction)
{
  if (m_exclusiveMode) {
    TRC_DBG("Dpa interface is in exclusiveMode");
    dpaTransaction->processFinish(DpaRequest::kCreated);
  }
  else if (m_dpaHandler) {
    try {
      m_dpaHandler->ExecuteDpaTransaction(*dpaTransaction);
    }
    catch (std::exception& e) {
      CATCH_EX("Error in ExecuteDpaTransaction: ", std::exception, e);
      dpaTransaction->processFinish(DpaRequest::kCreated);
    }
  }
  else {
    TRC_ERR("Dpa interface is not working");
    dpaTransaction->processFinish(DpaRequest::kCreated);
  }

  //Pet WatchDog
  watchDogPet();

}

void MessagingController::registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> asyncHandler)
{
  m_asyncHandler = asyncHandler;
  m_dpaHandler->RegisterAsyncMessageHandler([&](const DpaMessage& dpaMessage) {
    if (m_asyncHandler) {
      m_asyncHandler(dpaMessage);
    }
    else {
      TRC_WAR("Unregistered asyncHandler()");
    }
  });
}

//void MessagingController::unregisterAsyncDpaMessageHandler()
//{
//  m_asyncHandler = nullptr;
//}

MessagingController::MessagingController()
  :m_iqrfInterface(nullptr)
  , m_dpaHandler(nullptr)
  , m_dpaTransactionQueue(nullptr)
  , m_scheduler(nullptr)
{
  m_exclusiveMode = false;
}

void MessagingController::loadConfiguration(const std::string& cfgFileName)
{
  std::cout << std::endl << "Loading configuration file: " << PAR(cfgFileName);
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

MessagingController::~MessagingController()
{
}

void MessagingController::run(const std::string& cfgFileName)
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

  std::unique_lock<std::mutex> lock(m_stopConditionMutex);
  while (m_running && ((std::chrono::system_clock::now() - m_lastRefreshTime)) < m_timeout)
    m_stopConditionVariable.wait_for(lock, m_timeout);

  try {
    stop();
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  TRC_LEAVE("");
}

void MessagingController::watchDogPet()
{
  std::unique_lock<std::mutex> lock(m_stopConditionMutex);
  m_lastRefreshTime = std::chrono::system_clock::now();
}

void MessagingController::exclusiveAccess(bool mode)
{
  TRC_ENTER(PAR(mode));
  if (mode == m_exclusiveMode)
    return;
  m_exclusiveMode = mode;
  if (mode) {
    TRC_INF("Exclusive mode enabled");
    m_exclusiveMode = mode;
    //stopClients();
    //stopScheduler();
    stopDpa();
  }
  else {
    TRC_INF("Exclusive mode disabled");
    startDpa();
    //startScheduler();
    //startClients();
  }
  TRC_LEAVE("");
}

IChannel* MessagingController::getIqrfInterface()
{
  return m_iqrfInterface;
}

///////// STARTS ////////////////////////////
void MessagingController::startTrace()
{
  auto fnd = m_componentMap.find("TracerFile");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      jutils::assertIsObject("", fnd->second.m_doc);

      m_traceFileName = jutils::getPossibleMemberAs<std::string>("TraceFileName", fnd->second.m_doc, "");
      m_traceFileSize = jutils::getPossibleMemberAs<int>("TraceFileSize", fnd->second.m_doc, 0);
      if (m_traceFileSize <= 0)
        m_traceFileSize = TRC_DEFAULT_FILE_MAXSIZE;

      TRC_START(m_traceFileName, m_traceFileSize);
      TRC_INF("Loaded configuration file: " << PAR(m_cfgFileName));
      TRC_INF("Opened trace file: " << PAR(m_traceFileName) << PAR(m_traceFileSize));

    }
    catch (std::exception &e) {
      std::cout << "Cannot create TracerFile: " << e.what() << std::endl;
    }
  }
}

void MessagingController::startIqrfIf()
{

  auto fnd = m_componentMap.find("IqrfInterface");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      jutils::assertIsObject("", fnd->second.m_doc);
      m_iqrfInterfaceName = jutils::getMemberAs<std::string>("IqrfInterface", fnd->second.m_doc);
      TRC_INF(PAR(m_iqrfInterfaceName));

      size_t found = m_iqrfInterfaceName.find("spi");
      if (found != std::string::npos)
        m_iqrfInterface = ant_new IqrfSpiChannel(m_iqrfInterfaceName);
      else
        m_iqrfInterface = ant_new IqrfCdcChannel(m_iqrfInterfaceName);

    }
    catch (std::exception &e) {
      CATCH_EX("Cannot create IqrfInterface: ", std::exception, e);
    }
  }

  m_dpaTransactionQueue = ant_new TaskQueue<DpaTransaction*>([&](DpaTransaction* trans) {
    executeDpaTransactionFunc(trans);
  });

}

void MessagingController::startUdp()
{
  try {
    m_udpMessaging = ant_new UdpMessaging(this);
    m_udpMessaging->start();
  }
  catch (std::exception &e) {
    CATCH_EX("Cannot create UdpMessaging ", std::exception, e);
  }
}

void MessagingController::startDpa()
{
  try {
    m_dpaHandler = ant_new DpaHandler(m_iqrfInterface);
    m_dpaHandler->Timeout(200);    // Default timeout is infinite
  }
  catch (std::exception& ae) {
    TRC_ERR("There was an error during DPA handler creation: " << ae.what());
  }
}

void MessagingController::startScheduler()
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

  m_scheduler->start();
  
  //Pet WatchDog at least from Scheduler if there is no DpaTransaction
  m_scheduler->registerMessageHandler("WatchDogPet", [this](const std::string& msg) {
    watchDogPet();
  });
  m_scheduler->scheduleTaskPeriodic("WatchDogPet", "WatchDogPet",
    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(m_watchDogTimeoutMilis/2)));

}

void MessagingController::loadSerializerComponent(const ComponentDescriptor& componentDescriptor)
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

void MessagingController::loadMessagingComponent(const ComponentDescriptor& componentDescriptor)
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
    const auto propertiesMember = jutils::getMember("Properties", *itr);
    const rapidjson::Value& properties = propertiesMember->value;
    jutils::assertIsObject("Properties{}", properties);

    //create instance
    CreateMessaging createMessaging = (CreateMessaging)getCreateFunction(componentDescriptor.m_componentName, true);
    std::unique_ptr<IMessaging> messaging = createMessaging(instanceName);

    //get properties
    messaging->update(properties);

    //register instance
    auto ret = m_messagings.insert(std::make_pair(messaging->getName(), std::move(messaging)));
    if (!ret.second) {
      THROW_EX(std::logic_error, "Duplicit Messaging configuration: " << NAME_PAR(name, ret.first->first));
    }
  }
}

void MessagingController::loadClientComponent(const ComponentDescriptor& componentDescriptor)
{
  if (componentDescriptor.m_interfaceName != "IClient")
    return;

  typedef std::unique_ptr<IClient>(*CreateClientService)(const std::string&);

  const auto instancesMember = jutils::getMember("Instances", componentDescriptor.m_doc);
  const rapidjson::Value& instances = instancesMember->value;
  jutils::assertIsArray("Instances[]", instances);

  for (auto itr = instances.Begin(); itr != instances.End(); ++itr) {
    //parse instance descriptor
    jutils::assertIsObject("Instances[]{}", *itr);
    std::string instanceName = jutils::getMemberAs<std::string>("Name", *itr);
    std::string messagingName = jutils::getMemberAs<std::string>("Messaging", *itr);
    std::vector<std::string> serializersVector = jutils::getMemberAsVector<std::string>("Serializers", *itr);
    const auto propertiesMember = jutils::getMember("Properties", *itr);
    const rapidjson::Value& properties = propertiesMember->value;
    jutils::assertIsObject("Properties{}", properties);

    //create instance
    CreateClientService createService = (CreateClientService)getCreateFunction(componentDescriptor.m_componentName, true);
    std::unique_ptr<IClient> client = createService(instanceName);
    client->setDaemon(this);

    //get messaging
    {
      auto found = m_messagings.find(messagingName);
      if (found != m_messagings.end()) {
        client->setMessaging(found->second.get());
      }
      else
        THROW_EX(std::logic_error, "Cannot find: " << PAR(messagingName));
    }

    //get serializers
    for (auto & serializerName : serializersVector) {
      auto ss = m_serializers.find(serializerName);
      if (ss != m_serializers.end())
        client->setSerializer(ss->second.get());
      else
        THROW_EX(std::logic_error, "Cannot find: " << PAR(serializerName));
    }

    //get properties
    client->update(properties);

    //register instance
    auto ret = m_clients.insert(std::make_pair(client->getClientName(), std::move(client)));
    if (!ret.second) {
      THROW_EX(std::logic_error, "Duplicit ClientService configuration: " << NAME_PAR(name, ret.first->first));
    }

  }
}

void MessagingController::startClients()
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

  //load client components
  for (const auto & mc : m_componentMap) {
    if (mc.second.m_enabled) {
      try {
        loadClientComponent(mc.second);
      }
      catch (std::exception &e) {
        CATCH_EX("Cannot create component" << NAME_PAR(componentName, mc.first), std::exception, e);
      }
    }
  }

  //start clients
  for (auto & cli : m_clients) {
    try {
      cli.second->start();
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot start " << NAME_PAR(client, cli.second->getClientName()), std::exception, e);
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

  TRC_LEAVE("");
}

void MessagingController::start()
{
  TRC_ENTER("");

  startTrace();
  startIqrfIf();
  startUdp();
  startDpa();
  startScheduler();
  startClients();

  TRC_INF("daemon started");
  TRC_LEAVE("");
}

///////// STOPS ////////////////////////////

void MessagingController::stopTrace()
{
}

void MessagingController::stopIqrfIf()
{
  delete m_dpaTransactionQueue;
  m_dpaTransactionQueue = nullptr;

  delete m_iqrfInterface;
  m_iqrfInterface = nullptr;
}

void MessagingController::stopUdp()
{
  m_udpMessaging->stop();
  delete m_udpMessaging;
  m_udpMessaging = nullptr;
}

void MessagingController::stopDpa()
{
  delete m_dpaHandler;
  m_dpaHandler = nullptr;
}

void MessagingController::stopClients()
{
  TRC_ENTER("");
  for (auto & cli : m_clients) {
    cli.second->stop();
  }
  m_clients.clear();

  for (auto & ms : m_messagings) {
    ms.second->stop();
  }
  m_messagings.clear();

  for (auto & sr : m_serializers) {
  }
  m_serializers.clear();

  TRC_LEAVE("");
}


void MessagingController::stopScheduler()
{
  m_scheduler->stop();
  delete m_scheduler;
}

void MessagingController::stop()
{
  TRC_ENTER("");
  TRC_INF("daemon stops");

  stopClients();
  stopScheduler();
  stopDpa();
  stopUdp();
  stopIqrfIf();
  stopTrace();

  TRC_LEAVE("");
}

void MessagingController::exit()
{
  TRC_INF("exiting ...");
  {
    std::unique_lock<std::mutex> lock(m_stopConditionMutex);
    m_running = false;
  }
  m_stopConditionVariable.notify_all();
}

//////////////// Launch
void * MessagingController::getCreateFunction(const std::string& componentName, bool mandatory) const
{
  std::string fname = ("__launch_create_");
  fname += componentName;
  return getFunction(fname, mandatory);
}

void * MessagingController::getFunction(const std::string& methodName, bool mandatory) const
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
