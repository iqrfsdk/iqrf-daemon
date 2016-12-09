#include "MessagingController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"

#include "IClient.h"
//TODO temporary here
#include "TestClient.h"

//TODO temporary here
#include "IMessaging.h"
#include "UdpMessaging.h"
#include "MqMessaging.h"
#include "MqttMessaging.h"
#include "SchedulerMessaging.h"

#include "SimpleSerializer.h"
#include "JsonSerializer.h"

#include "IqrfLogging.h"
#include "PlatformDep.h"

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

void MessagingController::registerAsyncDpaMessageHandler(std::function<void(const DpaMessage&)> asyncHandler)
{
  m_asyncHandler = asyncHandler;
  m_dpaHandler->RegisterAsyncMessageHandler([&](const DpaMessage& dpaMessage) {
    m_asyncHandler(dpaMessage);
  });
}

MessagingController::MessagingController(const std::string& iqrfPortName)
  :m_iqrfInterface(nullptr)
  ,m_dpaHandler(nullptr)
  ,m_dpaTransactionQueue(nullptr)
  ,m_iqrfPortName(iqrfPortName)
  ,m_scheduler(nullptr)
{
}

MessagingController::~MessagingController ()
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
  MqttMessaging* mqttMessaging = ant_new MqttMessaging();
  m_messagings.push_back(mqttMessaging);

  MqMessaging* mqMessaging = ant_new MqMessaging();
  m_messagings.push_back(mqMessaging);

  UdpMessaging* udpMessaging = ant_new UdpMessaging();
  udpMessaging->setDaemon(this);
  m_messagings.push_back(udpMessaging);

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

  /////////////////////
  for (auto cli : m_clients) {
    cli.second->start();
  }

  for (auto ms : m_messagings) {
    ms->start();
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

  for (auto ms : m_messagings) {
    ms->stop();
    delete ms;
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
    size_t found = m_iqrfPortName.find("spi");
    if (found != std::string::npos)
      m_iqrfInterface = ant_new IqrfSpiChannel(m_iqrfPortName);
    else
      m_iqrfInterface = ant_new IqrfCdcChannel(m_iqrfPortName);

    m_dpaHandler = ant_new DpaHandler(m_iqrfInterface);

    m_dpaHandler->Timeout(100);    // Default timeout is infinite
  }
  catch (std::exception& ae) {
    TRC_ERR("There was an error during DPA handler creation: " << ae.what());
  }

  m_dpaTransactionQueue = ant_new TaskQueue<DpaTransaction*>([&](DpaTransaction* trans) {
    executeDpaTransactionFunc(trans);
  });

  m_scheduler = ant_new SchedulerMessaging();

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
