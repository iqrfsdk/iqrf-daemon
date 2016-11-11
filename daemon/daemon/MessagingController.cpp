#include "MessagingController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"

//TODO temporary here
#include "IMessaging.h"
#include "UdpMessaging.h"
//#include "MqMessaging.h"
#include "IqrfappMqMessaging.h"
#include "MqttMessaging.h"

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

MessagingController::MessagingController(const std::string& iqrfPortName)
  :m_iqrfInterface(nullptr)
  ,m_dpaHandler(nullptr)
  ,m_dpaTransactionQueue(nullptr)
  ,m_iqrfPortName(iqrfPortName)
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

void MessagingController::startProtocols()
{
  TRC_ENTER("");
  //TODO
  IMessaging* udp = ant_new UdpMessaging();
  udp->setDaemon(this);
  udp->start();

  IMessaging* mqm = ant_new IqrfappMqMessaging();
  mqm->setDaemon(this);
  mqm->start();

  IMessaging* mqttm = ant_new MqttMessaging();
  mqttm->setDaemon(this);
  mqttm->start();

  TRC_LEAVE("");
}

void MessagingController::stopProtocols()
{
  TRC_ENTER("");
  for (auto prti = m_protocols.begin(); prti != m_protocols.end(); prti++) {
    IMessaging* prt = *prti;
    prt->stop();
    delete prt;
  }
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

    m_dpaHandler = new DpaHandler(m_iqrfInterface);
    //dpaHandler->RegisterAsyncMessageHandler(std::bind(&DpaLibraryDemo::UnexpectedMessage,
    //  this,
    //  std::placeholders::_1));

    m_dpaHandler->Timeout(100);    // Default timeout is infinite
  }
  catch (std::exception& ae) {
    std::cout << "There was an error during DPA handler creation: " << ae.what() << std::endl;
  }

  m_dpaTransactionQueue = ant_new TaskQueue<DpaTransaction*>([&](DpaTransaction* trans) {
    executeDpaTransactionFunc(trans);
  });

  startProtocols();

  std::cout << "daemon started" << std::endl;
  TRC_LEAVE("");
}

void MessagingController::stop()
{
  TRC_ENTER("");
  std::cout << "daemon stops" << std::endl;
  
  stopProtocols();

  //TODO unregister call-backs first ?
  delete m_iqrfInterface;
  delete m_dpaTransactionQueue;
  TRC_LEAVE("");
}

void MessagingController::exit()
{
  TRC_WAR("exiting ...");
  std::cout << "daemon exits" << std::endl;
  m_running = false;
}
