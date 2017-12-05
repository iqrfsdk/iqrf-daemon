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

#include "LaunchUtils.h"
#include "ProtocolBridgeClientService.h"
#include "DpaTransactionTask.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

INIT_COMPONENT(IService, ProtocolBridgeClientService)


const std::string SCHEDULED_GET_AND_PROCESS_DATA_TASK("SND_GET_AND_PROCESS_DATA_TASK");


ProtocolBridgeClientService::ProtocolBridgeClientService(const std::string &name)
	:m_name(name)
	, m_messaging(nullptr)
	, m_daemon(nullptr)
	, m_serializer(nullptr)
{
}

ProtocolBridgeClientService::~ProtocolBridgeClientService()
{
}

void ProtocolBridgeClientService::update(const rapidjson::Value& cfg)
{
	updateConfiguration(cfg);
}

//------------------------
void ProtocolBridgeClientService::updateConfiguration(const rapidjson::Value& cfg)
{
	TRC_ENTER("");
	jutils::assertIsObject("", cfg);

	// read config settings
	m_frcPeriod = jutils::getPossibleMemberAs<int>("FrcPeriod", cfg, m_frcPeriod);
  m_sleepPeriod = jutils::getPossibleMemberAs<int>("SleepPeriod", cfg, m_sleepPeriod);
  m_wmPeriod = jutils::getPossibleMemberAs<int>("WmPeriod", cfg, m_wmPeriod);

	// update watched Protocol Bridges
	m_watchedProtocolBridges.clear();

	const auto v = jutils::getMember("DpaRequestJsonPattern", cfg);
	if (!v->value.IsObject()) {
		THROW_EX(std::logic_error, "Expected: Json Object, detected: " << NAME_PAR(name, v->value.GetString()) << NAME_PAR(type, v->value.GetType()));
	}

	std::vector<int> vect = jutils::getMemberAsVector<int>("ProtocolBridges", cfg);
	for (int protoBridgeAddr : vect) {
		ProtocolBridgeJson protocolBridgeJson(v->value);
		protocolBridgeJson.setNadr(protoBridgeAddr);
		ProtocolBridgeSchd protoBridgeSchd(protocolBridgeJson, m_daemon->getScheduler());
		m_watchedProtocolBridges.emplace(protoBridgeAddr, protoBridgeSchd);
	}

	TRC_LEAVE("");
}

void ProtocolBridgeClientService::setDaemon(IDaemon* daemon)
{
	m_daemon = daemon;
}

void ProtocolBridgeClientService::setSerializer(ISerializer* serializer)
{
	m_serializer = serializer;
	m_messaging->registerMessageHandler([this](const ustring& msg) {
		this->handleMsgFromMessaging(msg);
	});
}

void ProtocolBridgeClientService::setMessaging(IMessaging* messaging)
{
	m_messaging = messaging;
}

void ProtocolBridgeClientService::start()
{
	TRC_ENTER("");

	//remove all possible configured tasks
	m_daemon->getScheduler()->removeAllMyTasks(getName());

	//register task handler
	m_daemon->getScheduler()->registerMessageHandler(m_name, [this](const std::string& task) {
		this->handleTaskFromScheduler(task);
	});

	// schedule task of get and process data from protocol bridges
	m_daemon->getScheduler()->scheduleTaskAt(
		getName(),
		SCHEDULED_GET_AND_PROCESS_DATA_TASK,
		std::chrono::system_clock::now()
	);

	// schedule periodic task of get and process data from protocol bridge
	m_daemon->getScheduler()->scheduleTaskPeriodic(
		getName(),
		SCHEDULED_GET_AND_PROCESS_DATA_TASK,
    std::chrono::seconds(m_frcPeriod)
	);

	TRC_INF("ProtocolBridgeClientService :" << PAR(m_name) << " started");
	TRC_LEAVE("");
}

void ProtocolBridgeClientService::stop()
{
	TRC_ENTER("");

	TRC_INF("ProtocolBridgeClientService :" << PAR(m_name) << " stopped");
	TRC_LEAVE("");
}

void ProtocolBridgeClientService::handleTaskFromScheduler(const std::string& task)
{
	TRC_ENTER("");
	TRC_DBG("==================================" << std::endl <<
		"Received from Scheduler: " << std::endl << task);

	if (task == SCHEDULED_GET_AND_PROCESS_DATA_TASK) {
		getAndProcessDataFromMeters(task);
	} else {
		TRC_ERR("Unknown task: " PAR(task));
	}
	TRC_LEAVE("");
}

void ProtocolBridgeClientService::handleMsgFromMessaging(const ustring& msg)
{
	TRC_ENTER("");
	TRC_DBG("==================================" << std::endl <<
		"Received from Messaging: " << std::endl << FORM_HEX(msg.data(), msg.size()));

	//get input message
	std::string msgs((const char*)msg.data(), msg.size());
	//TODO
	TRC_LEAVE("");
}

// returns list of status data of active bridges
std::map<uint8_t, ProtocolBridgeClientService::BridgeStatusData>
ProtocolBridgeClientService::getActiveBridgesStatusMap() {
	TRC_ENTER("");
	PrfFrc frc(PrfFrc::Cmd::SEND, PrfFrc::FrcType::GET_BYTE, 0x00, { 0x00, 0x00 });
	frc.setHwpid(0xFFFF);

	TRC_DBG("Request: " << std::endl << FORM_HEX(frc.getRequest().DpaPacketData(), frc.getRequest().GetLength()));

	DpaTransactionTask trans(frc);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
	TRC_DBG("Response: " << std::endl << FORM_HEX(frc.getResponse().DpaPacketData(), frc.getResponse().GetLength()));

	std::map<uint8_t, BridgeStatusData> activeBridgesStatusMap;

	for (uint8_t addr = 1; addr <= frc.FRC_MAX_NODE_BYTE; addr++) {
		uint8_t data = frc.getFrcData_Byte(addr);
		std::bitset<8> dataByte(data);

		if (dataByte.test(0)) {
			BridgeStatusData bridgeStatusData;
			bridgeStatusData.isOn = true;
			bridgeStatusData.isData = (dataByte.test(1)) ? true : false;
			bridgeStatusData.isNewVisible = (dataByte.test(2)) ? true : false;
			bridgeStatusData.isNewInvisible = (dataByte.test(3)) ? true : false;

			activeBridgesStatusMap.insert(std::pair<uint8_t, BridgeStatusData>(addr, bridgeStatusData));
		}
	}

	TRC_LEAVE("");
	return activeBridgesStatusMap;
}

// returns new visible meters indexes for specified Protocol Bridge
std::list<uint8_t> ProtocolBridgeClientService::getNewVisibleMetersIndexes(uint8_t bridgeAddress) {
	TRC_ENTER("");
	std::list<uint8_t> newVisibleMetersIndexes;

	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	bridge.commandGetNewVisible();
	bridge.setHwpid(0xFFFF);

	TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
	TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));
	
	if (result == 0) {
		ProtocolBridge::NewVisibleMetersResponse newVisibleMetersResponse = bridge.getNewVisibleMetersResponse();

		for (int byteIndex = 0; byteIndex < ProtocolBridge::VISIBLE_METERS_BITMAP_LEN; byteIndex++) {
			std::bitset<8> dataByte(newVisibleMetersResponse.bitmap[byteIndex]);

			for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
				if (dataByte.test(bitIndex)) {
					newVisibleMetersIndexes.push_back(byteIndex * 8 + bitIndex);
				}
			}
		}
	}
	else {
		TRC_ERR("No processing run, check transaction return status!");
	}
	
	TRC_LEAVE("");
	return newVisibleMetersIndexes;
}

// returns new invisible meters indexes for specified Protocol Bridge
std::list<uint8_t> ProtocolBridgeClientService::getNewInvisibleMetersIndexes(uint8_t bridgeAddress) {
	TRC_ENTER("");
	std::list<uint8_t> newInvisibleMetersIndexes;
	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	bridge.commandGetNewInvisible();
	bridge.setHwpid(0xFFFF);

	TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
	TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));

	if (result == 0) {
		ProtocolBridge::NewInvisibleMetersResponse newInvisibleMetersResponse = bridge.getNewInvisibleMetersResponse();

		for (int byteIndex = 0; byteIndex < ProtocolBridge::INVISIBLE_METERS_BITMAP_LEN; byteIndex++) {
			std::bitset<8> dataByte(newInvisibleMetersResponse.bitmap[byteIndex]);

			for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
				if (dataByte.test(bitIndex)) {
					newInvisibleMetersIndexes.push_back(byteIndex * 8 + bitIndex);
				}
			}
		}
	}
	else {
		TRC_ERR("No processing run, check transaction return status!");
	}

	TRC_LEAVE("");
	return newInvisibleMetersIndexes;
}

// returns indexes of meters with new data for specified Protocol Bridge
std::list<uint8_t> ProtocolBridgeClientService::getNewDataMetersIndexes(uint8_t bridgeAddress) {
	TRC_ENTER("");
	std::list<uint8_t> newDataMetersIndexes;
	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	bridge.commandGetNewDataInfo();
	bridge.setHwpid(0xFFFF);

	TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
	TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));

	if (result == 0) {
		ProtocolBridge::NewDataInfoResponse newDataInfoResponse = bridge.getNewDataInfoResponse();

		for (int byteIndex = 0; byteIndex < ProtocolBridge::NEW_DATA_INFO_BITMAP_LEN; byteIndex++) {
			std::bitset<8> dataByte(newDataInfoResponse.bitmap[byteIndex]);

			for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
				if (dataByte.test(bitIndex)) {
					newDataMetersIndexes.push_back(byteIndex * 8 + bitIndex);
				}
			}
		}
	}
	else {
		TRC_ERR("No processing run, check transaction return status!");
	}

	TRC_LEAVE("");
	return newDataMetersIndexes;
}

ProtocolBridge::FullPacketResponse ProtocolBridgeClientService::getFullPacketResponse(
	uint8_t bridgeAddress, 
	uint8_t meterIndex
) {
	TRC_ENTER("");
	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	bridge.commandGetFullPacket(meterIndex);
	bridge.setHwpid(0xFFFF);

	TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
	TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));

	TRC_LEAVE("");
	return bridge.getFullPacketResponse();
}

void ProtocolBridgeClientService::confirmVisibleMeters(
	uint8_t bridgeAddress, 
	std::list<uint8_t> newVisibleMetersIndexes
) {
	TRC_ENTER("");
	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	uint8_t confirmationBitmap[ProtocolBridge::CONFIRMATION_BITMAP_LEN];
	memset(confirmationBitmap, 0, ProtocolBridge::CONFIRMATION_BITMAP_LEN);

	// 00, 01, 02, 03
	int mapIndex = 0;

	for (uint8_t meterIndex : newVisibleMetersIndexes) {
		// test, if it is needed to increase map index and send command
		if (meterIndex > (mapIndex * 10 * 8 + 10 * 8 - 1) ) {
			bridge.commandGetVisibleConfirmation(mapIndex, confirmationBitmap);
			bridge.setHwpid(0xFFFF);

			TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

			DpaTransactionTask trans(bridge);
			m_daemon->executeDpaTransaction(trans);
			int result = trans.waitFinish();

			TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
			TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));

			memset(confirmationBitmap, 0, ProtocolBridge::CONFIRMATION_BITMAP_LEN);
			mapIndex++;
		}

		int byteIndex = (meterIndex - mapIndex * 10 * 8) / 8;
		int bitIndex = (meterIndex - mapIndex * 10 * 8) % 8;
		confirmationBitmap[byteIndex] |= 1 << bitIndex;
	}

	// send confirmation for last map index
	bridge.commandGetVisibleConfirmation(mapIndex, confirmationBitmap);
	bridge.setHwpid(0xFFFF);

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_LEAVE("");
}

void ProtocolBridgeClientService::confirmInvisibleMeters(
	uint8_t bridgeAddress,
	std::list<uint8_t> newInvisibleMetersIndexes
) {
	TRC_ENTER("");
	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	uint8_t confirmationBitmap[ProtocolBridge::CONFIRMATION_BITMAP_LEN];
	memset(confirmationBitmap, 0, ProtocolBridge::CONFIRMATION_BITMAP_LEN);

	// 00, 01, 02, 03
	int mapIndex = 0;

	for (uint8_t meterIndex : newInvisibleMetersIndexes) {
		// test, if it is needed to increase map index and send command
		if (meterIndex > (mapIndex * 10 * 8 + 10 * 8 - 1)) {
			bridge.commandGetInvisibleConfirmation(mapIndex, confirmationBitmap);
			bridge.setHwpid(0xFFFF);

			TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

			DpaTransactionTask trans(bridge);
			m_daemon->executeDpaTransaction(trans);
			int result = trans.waitFinish();

			TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
			TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));

			memset(confirmationBitmap, 0, ProtocolBridge::CONFIRMATION_BITMAP_LEN);
			mapIndex++;
		}

		int byteIndex = (meterIndex - mapIndex * 10 * 8) / 8;
		int bitIndex = (meterIndex - mapIndex * 10 * 8) % 8;
		confirmationBitmap[byteIndex] |= 1 << bitIndex;
	}

	// send confirmation for last map index
	bridge.commandGetInvisibleConfirmation(mapIndex, confirmationBitmap);
	bridge.setHwpid(0xFFFF);

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();
	TRC_LEAVE("");
}

// returns position of M-field = 2C 2D in the specified full packet response 
int getMFieldPosition(ProtocolBridge::FullPacketResponse fullPacketResponse) {
	TRC_ENTER("");
	int mFieldPos = -1;

	// position of 2D byte
	int firstBytePos = -1;

	for (int i = 0; i < fullPacketResponse.msgLen; i++) {
		if (fullPacketResponse.vmbusMsg[i] == 0x2C) {
			if ( (firstBytePos == (i - 1))  && (i != 0) ) {
				return firstBytePos;
			}
		}

		if (fullPacketResponse.vmbusMsg[i] == 0x2D) {
			firstBytePos = i;
		}
	}

	TRC_LEAVE("");
	return mFieldPos;
}

ProtocolBridgeClientService::PacketHeader ProtocolBridgeClientService::parseFullPacketResponse(
	ProtocolBridge::FullPacketResponse fullPacketResponse
) {
	TRC_ENTER("");
	int mFieldPos = getMFieldPosition(fullPacketResponse);
	if (mFieldPos == -1) {
		TRC_ERR("M-field not found in the full packet response: " << PAR(fullPacketResponse.vmbusMsg));
	}

	PacketHeader packetHeader;
	memcpy(packetHeader.manufacturer, fullPacketResponse.vmbusMsg + mFieldPos, MANUFACTURER_LEN);
	memcpy(packetHeader.address, fullPacketResponse.vmbusMsg + mFieldPos + 2, ADDRESS_LEN);

	TRC_LEAVE("");
	return packetHeader;
}

void ProtocolBridgeClientService::sendDataIntoAzure(
	PacketHeader packetHeader, uint8_t data[], int dataLen
) {
	TRC_ENTER("");
	//encode output message
	std::ostringstream os;
	
	os << "{\"manufacturer\": \"";
	for (int i = 0; i < MANUFACTURER_LEN; i++) {
		os << std::setfill('0') << std::setw(sizeof(uint8_t)*2) << std::uppercase << std::hex << (int)packetHeader.manufacturer[i] << " ";
	}
	os << "\",";

	os << "\"address\": \"";
	for (int i = 0; i < ADDRESS_LEN; i++) {
		os << std::setfill('0') << std::setw(sizeof(uint8_t) * 2) << std::uppercase << std::hex << (int)packetHeader.address[i] << " ";
	}
	os << "\",";

	os << "\"data\": \"";
	for (int i = 0; i < dataLen; i++) {
		os << std::setfill('0') << std::setw(sizeof(uint8_t) * 2) << std::uppercase << std::hex << (int)data[i] << " ";
	}
	os << "\"}";

	ustring msgu((unsigned char*)os.str().data(), os.str().size());
	m_messaging->sendMessage(msgu);
	TRC_LEAVE("");
}

void ProtocolBridgeClientService::sleepProtocolBridge(uint8_t bridgeAddress) {
	TRC_ENTER("");
	ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
	ProtocolBridge bridge = bridgeSchedule.getDpa();

	bridge.commandSleepNow(m_sleepPeriod);
	bridge.setHwpid(0xFFFF);

	TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

	DpaTransactionTask trans(bridge);
	m_daemon->executeDpaTransaction(trans);
	int result = trans.waitFinish();

	TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
	TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));
	TRC_LEAVE("");
}

// timeout for wmbus module, module is on for this time
void ProtocolBridgeClientService::timeoutWMBProtocolBridge(uint8_t bridgeAddress) {
  TRC_ENTER("");
  ProtocolBridgeSchd bridgeSchedule = m_watchedProtocolBridges.at(bridgeAddress);
  ProtocolBridge bridge = bridgeSchedule.getDpa();

  bridge.commandSetWmTimeout(m_wmPeriod);
  bridge.setHwpid(0xFFFF);

  TRC_DBG("Request: " << std::endl << FORM_HEX(bridge.getRequest().DpaPacketData(), bridge.getRequest().GetLength()));

  DpaTransactionTask trans(bridge);
  m_daemon->executeDpaTransaction(trans);
  int result = trans.waitFinish();

  TRC_DBG("Transaction status: " << NAME_PAR(STATUS, trans.getErrorStr()));
  TRC_DBG("Response: " << std::endl << FORM_HEX(bridge.getResponse().DpaPacketData(), bridge.getResponse().GetLength()));
  TRC_LEAVE("");
}

// gets data from meters, processes them and sends them into Azure
void ProtocolBridgeClientService::getAndProcessDataFromMeters(const std::string& task)
{
	TRC_ENTER("");
	// test ///////////////////////////////
	//ProtocolBridgeJson pm = m_watchedPm[0].getDpa();
	//
	//ustring umsg = {
	//  0x02, 0x00, 0x22, 0x80, 0x0f, 0x00, 0x00, 0x46,
	//  0x01, 0x01, 0x57, 0x01, 0x03, 0xff, 0x27, 0x46,
	//  0x0a, 0x02, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00,
	//  0x00, 0x00, 0xd5, 0x37
	//};
	//DpaMessage msg(umsg);
	//pm.parseResponse(umsg);
	/////////////////////////////////

	std::map<uint8_t, BridgeStatusData> activeBridgesStatusMap = getActiveBridgesStatusMap();

	//for (std::pair<uint8_t, BridgeStatusData> activeBridgesStatusData : activeBridgesStatusMap)
	for (std::map<uint8_t, BridgeStatusData>::iterator it = activeBridgesStatusMap.begin(); it != activeBridgesStatusMap.end(); it++) {

		std::list<uint8_t> newVisibleMetersIndexes;
		std::list<uint8_t> newInvisibleMetersIndexes;
		std::list<uint8_t> newDataMetersIndexes;

		if (it->second.isNewVisible) {
			newVisibleMetersIndexes = getNewVisibleMetersIndexes(it->first);
			// confirmation of visiblemeters
			confirmVisibleMeters(it->first, newVisibleMetersIndexes);
      // set wm module timeout
      timeoutWMBProtocolBridge(it->first);
		}

		if (it->second.isNewInvisible) {
			newInvisibleMetersIndexes = getNewInvisibleMetersIndexes(it->first);
			// confirmation of visible and invisible meters
			confirmInvisibleMeters(it->first, newInvisibleMetersIndexes);
		}

		if (it->second.isData) {
			newDataMetersIndexes = getNewDataMetersIndexes(it->first);
		}

		// getting full data packets for indexes of new data meters
		for (uint8_t meterIndex : newDataMetersIndexes) {

			ProtocolBridge::FullPacketResponse fullPacketResponse = getFullPacketResponse(
				it->first, meterIndex
			);
			
			// parsing of full packet response
			PacketHeader packetHeader = parseFullPacketResponse(fullPacketResponse);

			// sending parsed data into Azure
			sendDataIntoAzure(packetHeader, fullPacketResponse.vmbusMsg, fullPacketResponse.msgLen);
		}
	}
	
	// send Protocol Bridges into Sleeping mode
	for (std::pair<uint8_t, BridgeStatusData> activeBridgesStatusData : activeBridgesStatusMap) {
		sleepProtocolBridge(activeBridgesStatusData.first);
	}
	TRC_LEAVE("");
}
