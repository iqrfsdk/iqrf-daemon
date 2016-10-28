#include "MessagingController.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "DpaHandler.h"
#include "MessageQueue.h"

//TODO temporary here
#include "IMessaging.h"
#include "UdpMessaging.h"

#include "IqrfLogging.h"
#include "PlatformDep.h"

void MessagingController ::handleMessageFromIqrf(const ustring& iqrfMessage)
{
  TRC_DBG("Received from IQRF: " << std::endl << FORM_HEX(iqrfMessage.data(), iqrfMessage.size()));
  //TODO
  if (m_protocols.size() > 0) {
    IMessaging* protocol = *(m_protocols.begin());
    protocol->sendMessage(iqrfMessage);
  }
}

void MessagingController::executeDpaTransaction(DpaTransaction& dpaTransaction)
{
  //TODO queue
  //m_toIqrfMessageQueue->pushToQueue(message);
  m_dpaHandler->ExecuteDpaTransaction(dpaTransaction);
}

MessagingController::MessagingController(const std::string& iqrfPortName)
  :m_iqrfInterface(nullptr)
  ,m_dpaHandler(nullptr)
  ,m_toIqrfMessageQueue(nullptr)
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
  IMessaging* udp = ant_new UdpMessaging();
  udp->setDaemon(this);
  udp->start();
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

    m_dpaHandler->Timeout(10);    // Default timeout is infinite
  }
  catch (std::exception& ae) {
    std::cout << "There was an error during DPA handler creation: " << ae.what() << std::endl;
  }

  //m_toIqrfMessageQueue = ant_new MessageQueue([&](const ustring& iqrfMessage) {
  //  processDpa);
  
  m_iqrfInterface->registerReceiveFromHandler([&](const ustring& iqrfMessage) -> int {
    handleMessageFromIqrf(iqrfMessage);
    return 0;
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
  delete m_toIqrfMessageQueue;
  TRC_LEAVE("");
}

void MessagingController::exit()
{
  TRC_WAR("exiting ...");
  std::cout << "daemon exits" << std::endl;
  m_running = false;
}
