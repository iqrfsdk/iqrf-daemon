#include "IqrfLogging.h"
#include "MessagingController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"
#include "JsonUtils.h"

#include "IClient.h"
//TODO temporary here
#include "ClientServicePlain.h"

//TODO temporary here
#include "IMessaging.h"
#include "UdpMessaging.h"
#include "MqMessaging.h"
#include "MqttMessaging.h"
#include "Scheduler.h"

#include "SimpleSerializer.h"
#include "JsonSerializer.h"

#include "PlatformDep.h"

TRC_INIT()

using namespace rapidjson;

void MessagingController::executeDpaTransaction(DpaTransaction& dpaTransaction)
{
  m_dpaTransactionQueue->pushToQueue(&dpaTransaction);
}

void MessagingController::abortAllDpaTransactions()
{
  TRC_ENTER("");
  TRC_LEAVE("");
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
}

void MessagingController::insertMessaging(std::unique_ptr<IMessaging> messaging)
{
  auto ret = m_messagings.insert(std::make_pair(messaging->getName(), std::move(messaging)));
  if (!ret.second) {
    THROW_EX(std::logic_error, "Duplicit Messaging configuration: " << NAME_PAR(name,ret.first->first));
  }
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

void MessagingController::unregisterAsyncDpaMessageHandler()
{
  m_asyncHandler = nullptr;
}

MessagingController::MessagingController(const std::string& cfgFileName)
  :m_iqrfInterface(nullptr)
  , m_dpaHandler(nullptr)
  , m_dpaTransactionQueue(nullptr)
  , m_scheduler(nullptr)
  , m_cfgFileName(cfgFileName)
{
    std::cout << std::endl << "Loading configuration file: " << PAR(cfgFileName);

    m_exclusiveMode = false;

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
    const auto m = jutils::getMember("Components", m_configuration);
    const rapidjson::Value& vct = m->value;
    jutils::assertIsArray("Components", vct);

    for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
      jutils::assertIsObject("Components[]", *itr);
      std::string componentName = jutils::getMemberAs<std::string>("ComponentName", *itr);
      bool enabled = jutils::getMemberAs<bool>("Enabled", *itr);
      auto inserted = m_componentMap.insert(make_pair(componentName, ComponentDescriptor(componentName,enabled)));
      inserted.first->second.loadConfiguration(m_configurationDir);
    }
}

MessagingController::~MessagingController()
{
}

std::set<IMessaging*>& MessagingController::getSetOfMessaging()
{
  return m_protocols;
}

void MessagingController::registerMessaging(IMessaging& messaging)
{
  m_protocols.insert(&messaging);
}

void MessagingController::unregisterMessaging(IMessaging& messaging)
{
  m_protocols.erase(&messaging);
}

void MessagingController::watchDog()
{
  TRC_ENTER("");
  m_running = true;
  try {
    start();
    //TODO wait on condition until 3000
    while (m_running)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      //TODO
      //watch worker threads and possibly restart
    }
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  stop();
  TRC_LEAVE("");
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
    m_udpMessaging->setDaemon(this);
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
}

void MessagingController::startClients()
{
  TRC_ENTER("");

  ///////// Messagings ///////////////////////////////////
  //TODO load Messagings plugins

  auto fnd = m_componentMap.find("MqttMessaging");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      std::unique_ptr<MqttMessaging> uptr(ant_new MqttMessaging());
      uptr->updateConfiguration(fnd->second.m_doc);
      insertMessaging(std::move(uptr));
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot create MqttMessaging ", std::exception, e);
    }
  }

  fnd = m_componentMap.find("MqMessaging");
  if (fnd != m_componentMap.end() && fnd->second.m_enabled) {
    try {
      std::unique_ptr<MqMessaging> uptr(ant_new MqMessaging());
      //TODO
      //uptr->updateConfiguration(fnd->second.m_doc); 
      insertMessaging(std::move(uptr));
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot create MqMessaging ", std::exception, e);
    }
  }

  //MqMessaging* mqMessaging(nullptr);
  //try {
  //  std::unique_ptr<MqMessaging> uptr = std::unique_ptr<MqMessaging>(ant_new MqMessaging());
  //  mqMessaging = uptr.get();
  //  insertMessaging(std::move(uptr));
  //}
  //catch (std::exception &e) {
  //  CATCH_EX("Cannot create MqMessaging ", std::exception, e);
  //}

  ///////// Serializers ///////////////////////////////////
  //TODO load Serializers plugins
  DpaTaskSimpleSerializerFactory* simpleSerializer = ant_new DpaTaskSimpleSerializerFactory();
  m_serializers.push_back(simpleSerializer);

  DpaTaskJsonSerializerFactory* jsonSerializer = ant_new DpaTaskJsonSerializerFactory();
  m_serializers.push_back(jsonSerializer);

  //////// Clients //////////////////////////////////
  //TODO load Clients plugins
  auto found1 = m_messagings.find("MqMessaging");
  if (found1 != m_messagings.end()) {
    IClient* client1 = ant_new ClientServicePlain("ClientServicePlain1");
    client1->setDaemon(this);
    client1->setMessaging(found1->second.get());

    //multi-serializer support
    client1->setSerializer(simpleSerializer);
    client1->setSerializer(jsonSerializer);

    m_clients.insert(std::make_pair(client1->getClientName(), client1));
  }

  //TODO will be solved by ClientServicePlain cfg
  auto found2 = m_messagings.find("MqttMessaging");
  if (found2 != m_messagings.end()) {
    IClient* client2 = ant_new ClientServicePlain("ClientServicePlain2");
    client2->setDaemon(this);
    client2->setMessaging(found2->second.get());
    client2->setSerializer(jsonSerializer);
    m_clients.insert(std::make_pair(client2->getClientName(), client2));
  }

  /////////////////////
  for (auto cli : m_clients) {
    try {
      cli.second->start();
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot start " << NAME_PAR(client, cli.second->getClientName()), std::exception, e);
    }
  }

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
  for (auto cli : m_clients) {
    cli.second->stop();
    delete cli.second;
  }
  m_clients.clear();

  for (auto & ms : m_messagings) {
    ms.second->stop();
  }
  m_messagings.clear();

  for (auto sr : m_serializers) {
    delete sr;
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
  m_running = false;
}

/////////////////////////////////////

void ComponentDescriptor::loadConfiguration(const std::string configurationDir)
{
  std::ostringstream os;
  os << configurationDir << '/' << m_componentName << ".json";

  try {
    jutils::parseJsonFile(os.str(), m_doc);
  }
  catch (std::exception &e) {
    CATCH_EX("Cannot load file " << NAME_PAR(fname, os.str()), std::exception, e);
    std::cerr << std::endl << "Cannot load file " << NAME_PAR(fname, os.str()) << NAME_PAR(error, e.what()) << std::endl;
  }

}
