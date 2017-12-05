/**
 * Copyright 2017 IQRF Tech s.r.o.
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

#include "IMessaging.h"
#include "IScheduler.h"
#include "JsonSerializer.h"
#include "LaunchUtils.h"
#include "ThermometerService.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "JsonUtils.h"

// required macro for static initialization of this component
INIT_COMPONENT(IService, ThermometerService)

const std::string SEND_THERMOMETERS_READ("SEND_THERMOMETERS_READ");
const std::string THERMOMETER_SERVICE("ThermometerService");

ThermometerService::ThermometerService(const std::string & name)
  :m_name(name)
  , m_messaging(nullptr)
  , m_daemon(nullptr)
{
}

ThermometerService::~ThermometerService()
{
  // wait to lockable => finished tasks
  std::unique_lock<std::mutex> lck(m_mtx);
}

void ThermometerService::update(const rapidjson::Value& cfg)
{
  // loads configuration
  updateConfiguration(cfg);
}

void ThermometerService::updateConfiguration(const rapidjson::Value& cfg)
{
  TRC_ENTER("");
  jutils::assertIsObject("", cfg);

  // get a period in seconds of scheduled task
  int readPeriod = 0;
  readPeriod = jutils::getPossibleMemberAs<int>("ReadPeriod", cfg, readPeriod);

  // get node adresses of thermometers to be read
  {
    std::unique_lock<std::mutex> lck(m_mtx);
    m_readPeriod = readPeriod;
    std::vector<int> vect = jutils::getMemberAsVector<int>("Thermometers", cfg);
    for (int node : vect) {
      m_thermometers.insert(std::make_pair(node,Val()));
    }
  }

  TRC_LEAVE("");
}

void ThermometerService::setDaemon(IDaemon* daemon)
{
  // set daemon instance
  m_daemon = daemon;
}

void ThermometerService::setSerializer(ISerializer* serializer)
{
  // nothing to do here as we don't use serializer
}

void ThermometerService::setMessaging(IMessaging* messaging)
{
  // set messaging instance
  m_messaging = messaging;
}

void ThermometerService::start()
{
  TRC_ENTER("");

  // register handler method for msgs comming from messaging component
  m_messaging->registerMessageHandler([this](const ustring& msg) {
    this->handleMsgFromMessaging(msg);
  });

  // prepare scheduler
  // remove all possible tasks if any (can be there from Scheduler.json)
  m_daemon->getScheduler()->removeAllMyTasks(getName());

  //register handler method for scheduled task
  m_daemon->getScheduler()->registerMessageHandler(m_name, [this](const std::string& task) {
    this->handleTaskFromScheduler(task);
  });

  //schedule reading task
  scheduleReading();

  TRC_INF(PAR(m_name) << " started");

  TRC_LEAVE("");
}

void ThermometerService::stop()
{
  TRC_ENTER("");
  TRC_INF(PAR(m_name) << " stopped");
  TRC_LEAVE("");
}

void ThermometerService::handleTaskFromScheduler(const std::string& task)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from Scheduler: " << std::endl << task);

  if (SEND_THERMOMETERS_READ == task) {
    // we serve only read task
    processThermometersRead();
  }
  else {
    TRC_WAR("Unknown task: " << PAR(task));
  }
}

void ThermometerService::handleMsgFromMessaging(const IMessaging::ustring& msg)
{
  using namespace rapidjson;
  TRC_DBG("==================================" << std::endl <<
    "Received from Messaging: " << std::endl << FORM_HEX(msg.data(), msg.size()));

  //get input message
  std::string smsg((const char*)msg.data(), msg.size());
  std::string m_lastError = "OK"; //initiated result
  int readPeriod = 0;
  
  // rapid json object to parse expected JSON
  // we expect simple msg:
  // {"service":"ThermometerService", "period": 30}
  // there could be update of thermometers[] as well, but it is not implemented in this example
  Document doc;

  try {
    jutils::parseString(smsg, doc);
    jutils::assertIsObject("", doc);

    // just verify the msg it hax expected format
    std::string service = jutils::getMemberAs<std::string>("service", doc);
    if (service.empty() || service != THERMOMETER_SERVICE) {
      std::ostringstream os;
      os << "Unexpected: " << PAR(service);
      throw std::logic_error(os.str());
    }
    // get updated period
    readPeriod = jutils::getMemberAs<int>("period", doc);

  }
  catch (std::exception &e) {
    // something failed - put the reason to response
    m_lastError = e.what();
  }

  // the response to be send back
  std::string response;

  // encode the response via rapidjson functions
  Document::AllocatorType& alloc = doc.GetAllocator();
  rapidjson::Value v;
  v.SetString(m_lastError.c_str(), alloc);
  doc.AddMember("status", v, alloc);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  response = buffer.GetString();

  // send it back via messaging
  m_messaging->sendMessage(IMessaging::ustring((uint8_t*)response.data(), response.length()));

  // store updated period
  m_readPeriod = readPeriod;
  scheduleReading();
}

void ThermometerService::scheduleReading()
{
  // assure exclusive access
  std::unique_lock<std::mutex> lck(m_mtx);

  // remove previous scheduled task
  m_daemon->getScheduler()->removeTask(getName(), m_schdTaskHandle);
  //schedule with updated read period - min period is 1s else stop reading
  if (m_readPeriod >= 1) {
    m_schdTaskHandle = m_daemon->getScheduler()->scheduleTaskPeriodic(getName(),
      SEND_THERMOMETERS_READ, std::chrono::seconds(m_readPeriod));
  }
}

void ThermometerService::processThermometersRead()
{
  using namespace rapidjson;
  std::unique_lock<std::mutex> lck(m_mtx);

  for (auto & thm : m_thermometers) {

    // DPA message
    DpaMessage dpaRequest;
    // address
    dpaRequest.DpaPacket().DpaRequestPacket_t.NADR = thm.first;
    // embedded thermometer
    dpaRequest.DpaPacket().DpaRequestPacket_t.PNUM = PNUM_THERMOMETER;
    // command to run
    dpaRequest.DpaPacket().DpaRequestPacket_t.PCMD = CMD_THERMOMETER_READ;
    // hwpid
    dpaRequest.DpaPacket().DpaRequestPacket_t.HWPID = 0xFFFF;
    // set request data length
    dpaRequest.SetLength(sizeof(TDpaIFaceHeader));

    // Raw DPA access
    DpaRaw rawThm(dpaRequest);

    // timeout
    rawThm.setTimeout(m_timeout);

    // send it to coordinator
    DpaTransactionTask transRead(rawThm);
    m_daemon->executeDpaTransaction(transRead);
    int resultRead = transRead.waitFinish();

    TRC_DBG(">>>>>>>>>>>>>>>>>> Thermometer result: " << NAME_PAR(addr,thm.first) << NAME_PAR(TransactionError, transRead.getErrorStr()));

    const DpaMessage & response = rawThm.getResponse();

    switch (resultRead) {
    case 0:
    {
      // decode returned response
      uint16_t temp16 = response.DpaPacket().DpaResponsePacket_t.DpaMessage.PerThermometerRead_Response.SixteenthValue;

      int tempi;
      if (temp16 & 0x8000) { // negative value
        tempi = temp16 & 0x7FFF;
        tempi = -tempi;
      }
      else
        tempi = temp16;

      double temperature = tempi * 0.0625; // *1/16
      thm.second.value = temperature;
      thm.second.valid = true;
    }
    break;

    default:
      //other error
      thm.second.value = -273.15;
      thm.second.valid = false;
    }
  }

  // encode custom JSON response
  // in case we have configured two thermometers it sends:
  // {"service":"ThermometerService", "Thermometers": ["24.5", "23.5"]]}
  // if a thermometer doesn't responde it sends "NaN"
  Document doc;
  Document::AllocatorType& alloc = doc.GetAllocator();
  rapidjson::Value item;

  doc.SetObject();
  item.SetString(getName().c_str(), alloc);
  doc.AddMember("service", item, alloc);

  item.SetArray();
  for (auto & tmp : m_thermometers) {
    rapidjson::Value val;
    if (tmp.second.valid)
      val.SetString(std::to_string(tmp.second.value).c_str(), alloc);
    else
      val.SetString("NaN", alloc);
    item.PushBack(val, alloc);
  }
  doc.AddMember("Thermometers", item, alloc);

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  std::string response = buffer.GetString();

  // send the response
  m_messaging->sendMessage(IMessaging::ustring((uint8_t*)response.data(), response.length()));
}
