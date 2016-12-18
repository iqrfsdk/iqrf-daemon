#include "MessagingController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"
#include "JsonUtils.h"

#include "IClient.h"
//TODO temporary here
#include "TestClient.h"
#include "ClientService.h"

//TODO temporary here
#include "IMessaging.h"
#include "UdpMessaging.h"
#include "MqMessaging.h"
#include "MqttMessaging.h"
#include "Scheduler.h"

#include "SimpleSerializer.h"
#include "JsonSerializer.h"

#include "IqrfLogging.h"
#include "PlatformDep.h"

TRC_INIT()

using namespace rapidjson;

void MessagingController::executeDpaTransaction(DpaTransaction& dpaTransaction)
{
  m_dpaTransactionQueue->pushToQueue(&dpaTransaction);
  //TODO wait here for something
}

//called from task queue thread passed by lambda in task queue ctor
void MessagingController::executeDpaTransactionFunc(DpaTransaction* dpaTransaction)
{
  if (m_dpaHandler) {
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
    m_asyncHandler(dpaMessage);
  });
}

MessagingController::MessagingController(const std::string& cfgFileName)
  :m_iqrfInterface(nullptr)
  , m_dpaHandler(nullptr)
  , m_dpaTransactionQueue(nullptr)
  , m_scheduler(nullptr)
  , m_cfgFileName(cfgFileName)
{
    std::cout << std::endl << "Loading configuration file: " << PAR(cfgFileName);

    jutils::parseJsonFile(m_cfgFileName, m_configuration);
    jutils::assertIsObject("", m_configuration);
    
    // check cfg version
    // TODO major.minor ...
    std::string cfgVersion = jutils::getMemberAs<std::string>("Configuration", m_configuration);
    if (cfgVersion != m_cfgVersion) {
      THROW_EX(std::logic_error, "Unexpected configuration: " << PAR(cfgVersion) << "expected: " << m_cfgVersion);
    }

    m_traceFileName = jutils::getPossibleMemberAs<std::string>("TraceFileName", m_configuration, "");
    m_traceFileSize = jutils::getPossibleMemberAs<int>("TraceFileSize", m_configuration, 0);
    if (m_traceFileSize <= 0)
      m_traceFileSize = TRC_DEFAULT_FILE_MAXSIZE;

    TRC_START(m_traceFileName, m_traceFileSize);
    TRC_INF("Loaded configuration file: " << PAR(cfgFileName));
    TRC_INF("Opened trace file: " << PAR(m_traceFileName) << PAR(m_traceFileSize) );

    m_iqrfInterfaceName = jutils::getMemberAs<std::string>("IqrfInterface", m_configuration);
    TRC_INF(PAR(m_iqrfInterfaceName));

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

void MessagingController::startClients()
{
  TRC_ENTER("");

  ///////// Messagings ///////////////////////////////////
  //TODO load Messagings plugins
  MqttMessaging* mqttMessaging(nullptr);

  const auto m = jutils::getMember("MqttMessaging", m_configuration);
  const rapidjson::Value& vct = m->value;
  jutils::assertIsArray("MqttMessaging", vct);

  for (auto itr = vct.Begin(); itr != vct.End(); ++itr) {
    jutils::assertIsObject("", *itr);
    try {
      std::unique_ptr<MqttMessaging> uptr(ant_new MqttMessaging());
      uptr->updateConfiguration(*itr);
      mqttMessaging = uptr.get(); //TODO
      insertMessaging(std::move(uptr));
    }
    catch (std::exception &e) {
      CATCH_EX("Cannot create MqttMessaging ", std::exception, e);
    }
  }

  MqMessaging* mqMessaging(nullptr);
  try {
    std::unique_ptr<MqMessaging> uptr = std::unique_ptr<MqMessaging>(ant_new MqMessaging());
    mqMessaging = uptr.get();
    insertMessaging(std::move(uptr));
  }
  catch (std::exception &e) {
    CATCH_EX("Cannot create MqMessaging ", std::exception, e);
  }

  UdpMessaging* udpMessaging(nullptr);
  try {
    std::unique_ptr<UdpMessaging> uptr = std::unique_ptr<UdpMessaging>(ant_new UdpMessaging());
    udpMessaging = uptr.get();
    insertMessaging(std::move(uptr));
  }
  catch (std::exception &e) {
    CATCH_EX("Cannot create UdpMessaging ", std::exception, e);
  }

  ///////// Serializers ///////////////////////////////////
  //TODO load Serializers plugins
  DpaTaskSimpleSerializerFactory* simpleSerializer = ant_new DpaTaskSimpleSerializerFactory();
  m_serializers.push_back(simpleSerializer);

  DpaTaskJsonSerializerFactory* jsonSerializer = ant_new DpaTaskJsonSerializerFactory();
  m_serializers.push_back(jsonSerializer);

  //////// Clients //////////////////////////////////
  //TODO load clients plugins
  IClient* client1 = ant_new TestClient("TestClient1");
  client1->setDaemon(this);
  client1->setMessaging(mqttMessaging);
  client1->setSerializer(simpleSerializer);
  m_clients.insert(std::make_pair(client1->getClientName(), client1));

  IClient* client2 = ant_new TestClient("TestClient2");
  client2->setDaemon(this);
  client2->setMessaging(mqttMessaging);
  client2->setSerializer(jsonSerializer);
  m_clients.insert(std::make_pair(client2->getClientName(), client2));

  IClient* clientIqrfapp = ant_new TestClient("TestClientIqrfapp");
  clientIqrfapp->setDaemon(this);
  clientIqrfapp->setMessaging(mqMessaging);
  clientIqrfapp->setSerializer(simpleSerializer);
  m_clients.insert(std::make_pair(clientIqrfapp->getClientName(), clientIqrfapp));

  //IClient* clientService = ant_new ClientService("ClientService");
  //clientIqrfapp->setDaemon(this);
  //clientIqrfapp->setMessaging(mqMessaging);
  //clientIqrfapp->setSerializer(simpleSerializer);
  //m_clients.insert(std::make_pair(clientService->getClientName(), clientService));

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

void MessagingController::start()
{
  TRC_ENTER("");

  try {
    size_t found = m_iqrfInterfaceName.find("spi");
    if (found != std::string::npos)
      m_iqrfInterface = ant_new IqrfSpiChannel(m_iqrfInterfaceName);
    else
      m_iqrfInterface = ant_new IqrfCdcChannel(m_iqrfInterfaceName);

    m_dpaHandler = ant_new DpaHandler(m_iqrfInterface);

    m_dpaHandler->Timeout(100);    // Default timeout is infinite
  }
  catch (std::exception& ae) {
    TRC_ERR("There was an error during DPA handler creation: " << ae.what());
  }

  m_dpaTransactionQueue = ant_new TaskQueue<DpaTransaction*>([&](DpaTransaction* trans) {
    executeDpaTransactionFunc(trans);
  });

  Scheduler* schd = ant_new Scheduler();
  m_scheduler = schd;
  schd->updateConfiguration(m_configuration);

  startClients();

  m_scheduler->start();

  TRC_INF("daemon started");
  TRC_LEAVE("");
}

void MessagingController::stop()
{
  TRC_ENTER("");
  TRC_INF("daemon stops");

  m_scheduler->stop();

  stopClients();

  delete m_scheduler;

  //TODO unregister call-backs first ?
  delete m_iqrfInterface;
  delete m_dpaTransactionQueue;
  delete m_dpaHandler;

  TRC_LEAVE("");
}

void MessagingController::exit()
{
  TRC_INF("exiting ...");
  m_running = false;
}
