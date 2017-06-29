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

#include "ProtocolBridge.h"
#include "IqrfLogging.h"

const std::string ProtocolBridge::PRF_NAME("ProtocolBridge");

const std::string STR_CMD_SYSINFO("SYSINFO");
const std::string STR_CMD_GET_NEW_VISIBLE("GET_NEW_VISIBLE");
const std::string STR_CMD_GET_NEW_INVISIBLE("GET_NEW_INVISIBLE");
const std::string STR_CMD_GET_ALL_VISIBLE("GET_ALL_VISIBLE");
const std::string STR_CMD_GET_NEW_DATA_INFO("GET_NEW_DATA_INFO");
const std::string STR_CMD_GET_COMPACT_PACKET("GET_COMPACT_PACKET");
const std::string STR_CMD_GET_FULL_PACKET("GET_FULL_PACKET");
const std::string STR_CMD_GET_VISIBLE_CONFIRMATION("GET_VISIBLE_CONFIRMATION");
const std::string STR_CMD_GET_INVISIBLE_CONFIRMATION("GET_INVISIBLE_CONFIRMATION");
const std::string STR_CMD_SLEEP_NOW("SLEEP_NOW");
const std::string STR_CMD_DELETE_ID("DELETE_ID");
const std::string STR_CMD_DELETE_ALL("DELETE_ALL");
const std::string STR_CMD_SET_SLEEP_INTERVAL("SET_SLEEP_INTERVAL");
const std::string STR_CMD_SET_WM_TIMEOUT("SET_WM_TIMEOUT");
const std::string STR_CMD_SET_WM_ALL_READED_TIMEOUT("SET_WM_ALL_READED_TIMEOUT");
const std::string STR_CMD_SET_NODE_TIMEOUT("SET_NODE_TIMEOUT");
const std::string STR_CMD_SET_ANY_PACKET_MODE("SET_ANY_PACKET_MODE");


const uint8_t PNUM_PROTOCOL_BRIDGE(0x26);


ProtocolBridge::ProtocolBridge()
	: DpaTask(PRF_NAME, PNUM_PROTOCOL_BRIDGE)
{
	setCmd(Cmd::SYSINFO);
}

ProtocolBridge::~ProtocolBridge()
{
}


void ProtocolBridge::commandSysinfo() {
	setCmd(Cmd::SYSINFO);
	m_request.SetLength(sizeof(TDpaIFaceHeader));
}

void ProtocolBridge::commandGetNewVisible() {
	setCmd(Cmd::GET_NEW_VISIBLE);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x01;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = 0x00;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2] = 0x00;
	m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
}

void ProtocolBridge::commandGetNewInvisible() {
	setCmd(Cmd::GET_NEW_INVISIBLE);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x02;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = 0x00;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2] = 0x00;
	m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
}

void ProtocolBridge::commandGetAllVisible() 
{
	setCmd(Cmd::GET_ALL_VISIBLE);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x06;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = 0x00;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2] = 0x00;
	m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
}

void ProtocolBridge::commandGetNewDataInfo() {
	setCmd(Cmd::GET_NEW_DATA_INFO);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x03;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = 0x00;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2] = 0x00;
	m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
}

void ProtocolBridge::commandGetCompactPacket(uint16_t meterIndex) {
	setCmd(Cmd::GET_COMPACT_PACKET);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x04;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = meterIndex >> 8;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2] = meterIndex & 0xff;
	m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
}

void ProtocolBridge::commandGetFullPacket(uint16_t meterIndex) {
	setCmd(Cmd::GET_FULL_PACKET);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x05;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = meterIndex >> 8;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2] = meterIndex & 0xff;
	m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
}

void ProtocolBridge::commandGetVisibleConfirmation(uint8_t index, uint8_t map[]) {
	setCmd(Cmd::GET_VISIBLE_CONFIRMATION);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x01;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = index;
	
	for (int i = 0; i < CONFIRMATION_BITMAP_LEN; i++) {
		m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2 + i] = map[i];
	}

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 2 + CONFIRMATION_BITMAP_LEN);
}

void ProtocolBridge::commandGetInvisibleConfirmation(uint8_t index, uint8_t map[]) {
	setCmd(Cmd::GET_INVISIBLE_CONFIRMATION);
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[0] = 0x02;
	m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[1] = index;

	for (int i = 0; i < CONFIRMATION_BITMAP_LEN; i++) {
		m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData[2 + i] = map[i];
	}

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 2 + CONFIRMATION_BITMAP_LEN);
}

void ProtocolBridge::commandSleepNow(uint16_t sleepTime) {
	setCmd(Cmd::SLEEP_NOW);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x01;
	p[1] = sleepTime >> 8;
	p[2] = sleepTime & 0xff;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandDeleteId(uint16_t meterIndex) {
	setCmd(Cmd::DELETE_ID);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x02;
	p[1] = meterIndex >> 8;
	p[2] = meterIndex & 0xff;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandDeleteAll() {
	setCmd(Cmd::DELETE_ALL);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x03;
	p[1] = 0x00;
	p[2] = 0x00;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandSetSleepInterval(uint16_t sleepInterval) {
	setCmd(Cmd::SET_SLEEP_INTERVAL);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x01;
	p[1] = sleepInterval >> 8;
	p[2] = sleepInterval & 0xff;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandSetWmTimeout(uint16_t wmTimeout) {
	setCmd(Cmd::SET_WM_TIMEOUT);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x02;
	p[1] = wmTimeout >> 8;
	p[2] = wmTimeout & 0xff;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandSetWmAllReadedTimeout(uint16_t wmTimeout) {
	setCmd(Cmd::SET_WM_ALL_READED_TIMEOUT);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x03;
	p[1] = wmTimeout >> 8;
	p[2] = wmTimeout & 0xff;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandSetNodeTimeout(uint16_t wmTimeout) {
	setCmd(Cmd::SET_NODE_TIMEOUT);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x04;
	p[1] = wmTimeout >> 8;
	p[2] = wmTimeout & 0xff;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}

void ProtocolBridge::commandSetAnyPacketMode(bool enable) {
	setCmd(Cmd::SET_WM_ALL_READED_TIMEOUT);
	uint8_t* p = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
	p[0] = 0x05;
	p[1] = 0x00;
	p[2] = enable? 0x01 : 0x00;

	uint8_t checkSum = 0xff + (p[0] - 0x01) + (p[1] - 0x01) + (p[2] - 0x01);
	p[3] = checkSum;

	m_request.SetLength(sizeof(TDpaIFaceHeader) + 4);
}


// PARSING HELPER METHODS
void ProtocolBridge::parseSysinfoResponse(const uint8_t* pData) {
	for (int i = 0; i < SYSINFO_SERIAL_NUMBER_LEN; i++) {
		sysinfoResponse.serialNumber[i] = pData[1 + i];
	}

	int packetPos = SYSINFO_SERIAL_NUMBER_LEN + 1;

	sysinfoResponse.visibleMetersNum = pData[packetPos];
	sysinfoResponse.visibleMetersNum <<= 8;
	sysinfoResponse.visibleMetersNum |= pData[packetPos + 1];

	packetPos += 2;
	sysinfoResponse.readedMetersNum = pData[packetPos];
	sysinfoResponse.readedMetersNum <<= 8;
	sysinfoResponse.readedMetersNum |= pData[packetPos + 1];

	packetPos += 2;

	uint16_t tmp = pData[1];

	tmp = pData[packetPos];
	tmp <<= 8;
	tmp |= pData[packetPos + 1];
	sysinfoResponse.temperature = (double)tmp * 0.0625;

	packetPos += 2;
	tmp = pData[packetPos];
	tmp <<= 8;
	tmp |= pData[packetPos + 1];
	sysinfoResponse.powerVoltage = (double)tmp * 0.806;

	packetPos += 2;
	for (int i = 0; i < SYSINFO_FIRMWARE_VERSION_LEN; i++) {
		sysinfoResponse.firmwareVersion[i] = pData[packetPos + i];
	}

	packetPos += SYSINFO_FIRMWARE_VERSION_LEN;
	sysinfoResponse.sleepCount = pData[packetPos];
	sysinfoResponse.sleepCount <<= 8;
	sysinfoResponse.sleepCount |= pData[packetPos + 1];

	packetPos += 2;
	sysinfoResponse.wmbTimeout = pData[packetPos];
	sysinfoResponse.wmbTimeout <<= 8;
	sysinfoResponse.wmbTimeout |= pData[packetPos + 1];

	packetPos += 2;
	sysinfoResponse.wmbAllReaedTimeout = pData[packetPos];
	sysinfoResponse.wmbAllReaedTimeout <<= 8;
	sysinfoResponse.wmbAllReaedTimeout |= pData[packetPos + 1];

	packetPos += 2;
	sysinfoResponse.nodeTimeout = pData[packetPos];
	sysinfoResponse.nodeTimeout <<= 8;
	sysinfoResponse.nodeTimeout |= pData[packetPos + 1];

	packetPos += 2;
	sysinfoResponse.anyPacketNode = (pData[packetPos] == 1) ? true : false;

	packetPos++;
	sysinfoResponse.countToInvisible = pData[packetPos];

	packetPos++;
	sysinfoResponse.checksum = pData[packetPos];
}

void ProtocolBridge::parseGetNewVisibleResponse(const uint8_t* pData) {
	newVisibleMetersResponse.metersNum = pData[2];
	newVisibleMetersResponse.metersNum <<= 8;
	newVisibleMetersResponse.metersNum |= pData[3];

	for ( int i = 0; i < VISIBLE_METERS_BITMAP_LEN; i++ ) {
		newVisibleMetersResponse.bitmap[i] = pData[4 + i];
	}
}

void ProtocolBridge::parseGetNewInvisibleResponse(const uint8_t* pData) {
	newInvisibleMetersResponse.metersNum = pData[2];
	newInvisibleMetersResponse.metersNum <<= 8;
	newInvisibleMetersResponse.metersNum |= pData[3];

	for ( int i = 0; i < INVISIBLE_METERS_BITMAP_LEN; i++ ) {
		newInvisibleMetersResponse.bitmap[i] = pData[4 + i];
	}
}

void ProtocolBridge::parseGetAllVisibleResponse(const uint8_t* pData) {
	allVisibleMetersResponse.metersNum = pData[2];
	allVisibleMetersResponse.metersNum <<= 8;
	allVisibleMetersResponse.metersNum |= pData[3];

	for ( int i = 0; i < ALL_VISIBLE_METERS_BITMAP_LEN; i++ ) {
		allVisibleMetersResponse.bitmap[i] = pData[4 + i];
	}
}

void ProtocolBridge::parseGetNewDataInfoResponse(const uint8_t* pData) {
	newDataInfoResponse.metersNum = pData[2];
	newDataInfoResponse.metersNum <<= 8;
	newDataInfoResponse.metersNum |= pData[3];

	for (int i = 0; i < NEW_DATA_INFO_BITMAP_LEN; i++) {
		newDataInfoResponse.bitmap[i] = pData[4 + i];
	}
}

void ProtocolBridge::parseGetCompactPacketResponse(const uint8_t* pData) {
	compactPacketResponse.msgLen = pData[0] - 1 - 2 - 4;
	compactPacketResponse.meterIndex = pData[2];
	compactPacketResponse.meterIndex <<= 8;
	compactPacketResponse.meterIndex |= pData[3];

	for (int i = 0; i < compactPacketResponse.msgLen; i++) {
		compactPacketResponse.vmbusMsg[i] = pData[3 + i];
	}

	compactPacketResponse.rssi = pData[3 + compactPacketResponse.msgLen];
	compactPacketResponse.crc = pData[4 + compactPacketResponse.msgLen];
	compactPacketResponse.afterReceiptWmbusChecksum = pData[5 + compactPacketResponse.msgLen];
	compactPacketResponse.countedWmbusChecksum = pData[6 + compactPacketResponse.msgLen];
}

void ProtocolBridge::parseGetFullPacketResponse(const uint8_t* pData) {
	fullPacketResponse.msgLen = pData[0] - 1 - 2 - 4;
	fullPacketResponse.meterIndex = pData[2];
	fullPacketResponse.meterIndex <<= 8;
	fullPacketResponse.meterIndex |= pData[3];

	for (int i = 0; i < fullPacketResponse.msgLen; i++) {
		fullPacketResponse.vmbusMsg[i] = pData[3 + i];
	}

	fullPacketResponse.rssi = pData[3 + fullPacketResponse.msgLen];
	fullPacketResponse.crc = pData[4 + fullPacketResponse.msgLen];
	fullPacketResponse.afterReceiptWmbusChecksum = pData[5 + fullPacketResponse.msgLen];
	fullPacketResponse.countedWmbusChecksum = pData[6 + fullPacketResponse.msgLen];
}

void ProtocolBridge::parseGetVisibleConfirmationResponse(const uint8_t* pData) {
	visibleConfirmationResponse.confirmedNum = pData[3];
	visibleConfirmationResponse.actionResult = (pData[4] == 1) ? true : false;
}

void ProtocolBridge::parseGetInvisibleConfirmationResponse(const uint8_t* pData) {
	invisibleConfirmationResponse.confirmedNum = pData[3];
	invisibleConfirmationResponse.actionResult = (pData[4] == 1) ? true : false;
}

void ProtocolBridge::parseSleepNowResponse(const uint8_t* pData) {
	sleepNowResponse.requestValue = pData[3];
	sleepNowResponse.requestValue <<= 8;
	sleepNowResponse.requestValue |= pData[4];
}

void ProtocolBridge::parseDeleteIdResponse(const uint8_t* pData) {
	deleteIdReponse.meterIndex = pData[3];
	deleteIdReponse.meterIndex <<= 8;
	deleteIdReponse.meterIndex |= pData[4];
}

void ProtocolBridge::parseDeleteAllResponse(const uint8_t* pData) {
	deleteAllResponse.actionResult = (pData[3] == 1) ? true : false;
}

void ProtocolBridge::parseSetNodeTimeoutResponse(const uint8_t* pData) {
	setNodeTimeoutResponse.requestValue = pData[3];
	setNodeTimeoutResponse.requestValue <<= 8;
	setNodeTimeoutResponse.requestValue |= pData[4];
	setNodeTimeoutResponse.actionResult = (pData[5] == 1) ? true : false;
}

void ProtocolBridge::parseSetAnyPacketModeResponse(const uint8_t* pData) {
	setAnyPacketModeResponse.requestValue = pData[3];
	setAnyPacketModeResponse.requestValue <<= 8;
	setAnyPacketModeResponse.requestValue |= pData[4];
	setAnyPacketModeResponse.actionResult = (pData[5] == 1) ? true : false;
}

void ProtocolBridge::parseResponse(const DpaMessage& response) {
	Cmd cmd = getCmd();
	
	switch ( cmd ) {
		case Cmd::SYSINFO: 
			parseSysinfoResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_NEW_VISIBLE:
			parseGetNewVisibleResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_NEW_INVISIBLE:
			parseGetNewInvisibleResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_ALL_VISIBLE:
			parseGetAllVisibleResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_NEW_DATA_INFO:
			parseGetNewDataInfoResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_COMPACT_PACKET:
			parseGetCompactPacketResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_FULL_PACKET:
			parseGetFullPacketResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::GET_VISIBLE_CONFIRMATION:
			parseGetVisibleConfirmationResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;
		
		case Cmd::GET_INVISIBLE_CONFIRMATION:
			parseGetInvisibleConfirmationResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::SLEEP_NOW:
			parseSleepNowResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::DELETE_ID:
			parseDeleteIdResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::DELETE_ALL:
			parseDeleteAllResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;
		
		case Cmd::SET_NODE_TIMEOUT:
			parseSetNodeTimeoutResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		case Cmd::SET_ANY_PACKET_MODE:
			parseSetAnyPacketModeResponse(
				response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData
			);
			break;

		default:
			THROW_EX(std::logic_error, "Invalid command: " << PAR((uint8_t)cmd));
	}
}


void ProtocolBridge::parseCommand(const std::string& command) {
	if ( command == STR_CMD_SYSINFO ) {
		setCmd(Cmd::SYSINFO);
	} 
	else if ( command == STR_CMD_GET_NEW_VISIBLE ) {
		setCmd(Cmd::GET_NEW_VISIBLE);
	} 
	else if (command == STR_CMD_GET_NEW_INVISIBLE) {
		setCmd(Cmd::GET_NEW_INVISIBLE);
	} 
	else if (command == STR_CMD_GET_ALL_VISIBLE) {
		setCmd(Cmd::GET_ALL_VISIBLE);
	} 
	else if (command == STR_CMD_GET_NEW_DATA_INFO) {
		setCmd(Cmd::GET_NEW_DATA_INFO);
	} 
	else if (command == STR_CMD_GET_COMPACT_PACKET) {
		setCmd(Cmd::GET_COMPACT_PACKET);
	} 
	else if (command == STR_CMD_GET_FULL_PACKET) {
		setCmd(Cmd::GET_FULL_PACKET);
	} 
	else if (command == STR_CMD_GET_VISIBLE_CONFIRMATION) {
		setCmd(Cmd::GET_VISIBLE_CONFIRMATION);
	} 
	else if (command == STR_CMD_GET_INVISIBLE_CONFIRMATION) {
		setCmd(Cmd::GET_INVISIBLE_CONFIRMATION);
	} 
	else if (command == STR_CMD_SLEEP_NOW) {
		setCmd(Cmd::SLEEP_NOW);
	} 
	else if (command == STR_CMD_DELETE_ID) {
		setCmd(Cmd::DELETE_ID);
	} 
	else if (command == STR_CMD_DELETE_ALL) {
		setCmd(Cmd::DELETE_ALL);
	}
	else if (command == STR_CMD_SET_SLEEP_INTERVAL) {
		setCmd(Cmd::SET_SLEEP_INTERVAL);
	}
	else if (command == STR_CMD_SET_WM_TIMEOUT) {
		setCmd(Cmd::SET_WM_TIMEOUT);
	}
	else if (command == STR_CMD_SET_WM_ALL_READED_TIMEOUT) {
		setCmd(Cmd::SET_WM_ALL_READED_TIMEOUT);
	}
	else if (command == STR_CMD_SET_NODE_TIMEOUT) {
		setCmd(Cmd::SET_NODE_TIMEOUT);
	}
	else if (command == STR_CMD_SET_ANY_PACKET_MODE) {
		setCmd(Cmd::SET_ANY_PACKET_MODE);
	}
	else {
		THROW_EX(std::logic_error, "Invalid command: " << PAR(command));
	}
}

const std::string& ProtocolBridge::encodeCommand() const {
	switch ( getCmd() ) {
		case Cmd::SYSINFO:
			return STR_CMD_SYSINFO;
		case Cmd::GET_NEW_VISIBLE:
			return STR_CMD_GET_NEW_VISIBLE;
		case Cmd::GET_NEW_INVISIBLE:
			return STR_CMD_GET_NEW_INVISIBLE;
		case Cmd::GET_ALL_VISIBLE:
			return STR_CMD_GET_ALL_VISIBLE;
		case Cmd::GET_NEW_DATA_INFO:
			return STR_CMD_GET_NEW_DATA_INFO;
		case Cmd::GET_COMPACT_PACKET:
			return STR_CMD_GET_COMPACT_PACKET;
		case Cmd::GET_FULL_PACKET:
			return STR_CMD_GET_FULL_PACKET;
		case Cmd::GET_VISIBLE_CONFIRMATION:
			return STR_CMD_GET_VISIBLE_CONFIRMATION;
		case Cmd::GET_INVISIBLE_CONFIRMATION:
			return STR_CMD_GET_INVISIBLE_CONFIRMATION;
		case Cmd::SLEEP_NOW:
			return STR_CMD_SLEEP_NOW;
		case Cmd::DELETE_ID:
			return STR_CMD_DELETE_ID;
		case Cmd::DELETE_ALL:
			return STR_CMD_DELETE_ALL;
		case Cmd::SET_SLEEP_INTERVAL:
			return STR_CMD_SET_SLEEP_INTERVAL;
		case Cmd::SET_WM_TIMEOUT:
			return STR_CMD_SET_WM_TIMEOUT;
		case Cmd::SET_WM_ALL_READED_TIMEOUT:
			return STR_CMD_SET_WM_ALL_READED_TIMEOUT;
		case Cmd::SET_NODE_TIMEOUT:
			return STR_CMD_SET_NODE_TIMEOUT;
		case Cmd::SET_ANY_PACKET_MODE:
			return STR_CMD_SET_ANY_PACKET_MODE;
		default:
			THROW_EX(std::logic_error, "Invalid command: " << NAME_PAR(command, (uint8_t)getCmd()));
	}
}


ProtocolBridge::Cmd ProtocolBridge::getCmd() const {
	return m_cmd;
}

void ProtocolBridge::setCmd(ProtocolBridge::Cmd cmd) {
	switch ( cmd ) {
		case Cmd::SYSINFO:
			setPcmd(0x00);
			break;

		case Cmd::GET_NEW_VISIBLE:
		case Cmd::GET_NEW_INVISIBLE:
		case Cmd::GET_ALL_VISIBLE:
		case Cmd::GET_NEW_DATA_INFO:
		case Cmd::GET_COMPACT_PACKET:
		case Cmd::GET_FULL_PACKET:
			setPcmd(0x01);
			break;

		case Cmd::GET_VISIBLE_CONFIRMATION:
		case Cmd::GET_INVISIBLE_CONFIRMATION:
			setPcmd(0x03);
			break;

		case Cmd::SLEEP_NOW:
		case Cmd::DELETE_ID:
		case Cmd::DELETE_ALL:
			setPcmd(0x04);
			break;

		case Cmd::SET_SLEEP_INTERVAL:
		case Cmd::SET_WM_TIMEOUT:
		case Cmd::SET_WM_ALL_READED_TIMEOUT:
		case Cmd::SET_NODE_TIMEOUT:
		case Cmd::SET_ANY_PACKET_MODE:
			setPcmd(0x02);
			break;

		default:
			THROW_EX(std::logic_error, "Invalid command: " << PAR((uint8_t)cmd));
	}

	m_cmd = cmd;
}

ProtocolBridgeJson::ProtocolBridgeJson()
	: ProtocolBridge()
{
}

ProtocolBridgeJson::ProtocolBridgeJson(const ProtocolBridgeJson& o)
	: PrfCommonJson(o), ProtocolBridge(o)
{
}

ProtocolBridgeJson::ProtocolBridgeJson(const rapidjson::Value& val)
{
	parseRequestJson(val, *this);
}

// converts specified bytes to hexa string
static std::string bytesToHexaString(uint8_t data[], int len) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0');

	for ( int i = 0; i < len; i++ ) {
		ss << std::hex << std::setw(2) << data[i];
		if (i != (len - 1)) {
			ss << ".";
		}
	}
		
	return ss.str();
}

void ProtocolBridgeJson::setNadr(uint16_t nadr) {
	encodeHexaNum(m_nadr, nadr);
	this->setAddress(nadr);
	m_has_nadr = true;
}

void ProtocolBridgeJson::encodeSysinfoResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	SysinfoResponse sysinfoResponse = getSysinfoResponse();

	std::string serialNumberStr = bytesToHexaString(sysinfoResponse.serialNumber, SYSINFO_SERIAL_NUMBER_LEN);
	v.SetString(serialNumberStr.c_str(), serialNumberStr.length(), alloc);
	m_doc.AddMember("SerialNumber", v, alloc);

	v = sysinfoResponse.visibleMetersNum;
	m_doc.AddMember("VisibleMetersNum", v, alloc);

	v = sysinfoResponse.readedMetersNum;
	m_doc.AddMember("ReadedMetersNum", v, alloc);

	v = sysinfoResponse.temperature;
	m_doc.AddMember("Temperature", v, alloc);

	v = sysinfoResponse.powerVoltage;
	m_doc.AddMember("PowerVoltage", v, alloc);

	std::string firmwareVersionStr = bytesToHexaString(sysinfoResponse.firmwareVersion, SYSINFO_FIRMWARE_VERSION_LEN);
	v.SetString(firmwareVersionStr.c_str(), firmwareVersionStr.length(), alloc);
	m_doc.AddMember("FirmwareVersion", v, alloc);

	v = sysinfoResponse.sleepCount;
	m_doc.AddMember("SleepCount", v, alloc);

	v = sysinfoResponse.wmbTimeout;
	m_doc.AddMember("WmbTimeout", v, alloc);

	v = sysinfoResponse.wmbAllReaedTimeout;
	m_doc.AddMember("wmbAllReaedTimeout", v, alloc);

	v = sysinfoResponse.nodeTimeout;
	m_doc.AddMember("NodeTimeout", v, alloc);

	v = sysinfoResponse.anyPacketNode;
	m_doc.AddMember("AnyPacketNode", v, alloc);

	v = sysinfoResponse.countToInvisible;
	m_doc.AddMember("CountToInvisible", v, alloc);

	v = sysinfoResponse.checksum;
	m_doc.AddMember("Checksum", v, alloc);
}

void ProtocolBridgeJson::encodeGetNewVisibleResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	NewVisibleMetersResponse newVisibleMetersResponse = getNewVisibleMetersResponse();

	v = newVisibleMetersResponse.metersNum;
	m_doc.AddMember("MetersNum", v, alloc);

	std::string bitmapStr = bytesToHexaString(newVisibleMetersResponse.bitmap, VISIBLE_METERS_BITMAP_LEN);
	v.SetString(bitmapStr.c_str(), bitmapStr.length(), alloc);
	m_doc.AddMember("Bitmap", v, alloc);
}

void ProtocolBridgeJson::encodeGetNewInvisibleResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	NewInvisibleMetersResponse newInvisibleMetersResponse = getNewInvisibleMetersResponse();

	v = newInvisibleMetersResponse.metersNum;
	m_doc.AddMember("MetersNum", v, alloc);

	std::string bitmapStr = bytesToHexaString(newInvisibleMetersResponse.bitmap, INVISIBLE_METERS_BITMAP_LEN);
	v.SetString(bitmapStr.c_str(), bitmapStr.length(), alloc);
	m_doc.AddMember("Bitmap", v, alloc);
}

void ProtocolBridgeJson::encodeGetAllVisibleResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	AllVisibleMetersResponse allVisibleMetersResponse = getAllVisibleMetersResponse();

	v = allVisibleMetersResponse.metersNum;
	m_doc.AddMember("MetersNum", v, alloc);

	std::string bitmapStr = bytesToHexaString(allVisibleMetersResponse.bitmap, ALL_VISIBLE_METERS_BITMAP_LEN);
	v.SetString(bitmapStr.c_str(), bitmapStr.length(), alloc);
	m_doc.AddMember("Bitmap", v, alloc);
}

void ProtocolBridgeJson::encodeGetNewDataInfoResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	NewDataInfoResponse newDataInfoResponse = getNewDataInfoResponse();

	v = newDataInfoResponse.metersNum;
	m_doc.AddMember("MetersNum", v, alloc);

	std::string bitmapStr = bytesToHexaString(newDataInfoResponse.bitmap, NEW_DATA_INFO_BITMAP_LEN);
	v.SetString(bitmapStr.c_str(), bitmapStr.length(), alloc);
	m_doc.AddMember("Bitmap", v, alloc);
}

void ProtocolBridgeJson::encodeGetCompactPacketResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	CompactPacketResponse compactPacketResponse = getCompactPacketResponse();

	v = compactPacketResponse.meterIndex;
	m_doc.AddMember("MeterIndex", v, alloc);

	std::string vmbusMsgStr = bytesToHexaString(compactPacketResponse.vmbusMsg, VMBUS_MAX_FRAME_LEN);
	v.SetString(vmbusMsgStr.c_str(), vmbusMsgStr.length(), alloc);
	m_doc.AddMember("VmbusMsg", v, alloc);

	v = compactPacketResponse.msgLen;
	m_doc.AddMember("MsgLen", v, alloc);

	v = compactPacketResponse.rssi;
	m_doc.AddMember("Rssi", v, alloc);

	v = compactPacketResponse.crc;
	m_doc.AddMember("Crc", v, alloc);

	v = compactPacketResponse.afterReceiptWmbusChecksum;
	m_doc.AddMember("AfterReceiptWmbusChecksum", v, alloc);

	v = compactPacketResponse.countedWmbusChecksum;
	m_doc.AddMember("CountedWmbusChecksum", v, alloc);
}

void ProtocolBridgeJson::encodeGetFullPacketResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	FullPacketResponse fullPacketResponse = getFullPacketResponse();

	v = fullPacketResponse.meterIndex;
	m_doc.AddMember("MeterIndex", v, alloc);

	std::string vmbusMsgStr = bytesToHexaString(fullPacketResponse.vmbusMsg, VMBUS_MAX_FRAME_LEN);
	v.SetString(vmbusMsgStr.c_str(), vmbusMsgStr.length(), alloc);
	m_doc.AddMember("VmbusMsg", v, alloc);

	v = fullPacketResponse.msgLen;
	m_doc.AddMember("MsgLen", v, alloc);

	v = fullPacketResponse.rssi;
	m_doc.AddMember("Rssi", v, alloc);

	v = fullPacketResponse.crc;
	m_doc.AddMember("Crc", v, alloc);

	v = fullPacketResponse.afterReceiptWmbusChecksum;
	m_doc.AddMember("AfterReceiptWmbusChecksum", v, alloc);

	v = fullPacketResponse.countedWmbusChecksum;
	m_doc.AddMember("CountedWmbusChecksum", v, alloc);
}

void ProtocolBridgeJson::encodeGetVisibleConfirmationResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	VisibleConfirmationResponse visibleConfirmationResponse = getVisibleConfirmationResponse();

	v = visibleConfirmationResponse.confirmedNum;
	m_doc.AddMember("ConfirmedNum", v, alloc);

	v = visibleConfirmationResponse.actionResult;
	m_doc.AddMember("ActionResult", v, alloc);
}

void ProtocolBridgeJson::encodeGetInvisibleConfirmationResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	InvisibleConfirmationResponse invisibleConfirmationResponse = getInvisibleConfirmationResponse();

	v = invisibleConfirmationResponse.confirmedNum;
	m_doc.AddMember("ConfirmedNum", v, alloc);

	v = invisibleConfirmationResponse.actionResult;
	m_doc.AddMember("ActionResult", v, alloc);
}

void ProtocolBridgeJson::encodeSleepNowResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	SleepNowResponse sleepNowResponse = getSleepNowResponse();

	v = sleepNowResponse.requestValue;
	m_doc.AddMember("RequestValue", v, alloc);
}

void ProtocolBridgeJson::encodeDeleteIdResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	DeleteIdResponse deleteIdResponse = getDeleteIdResponse();

	v = deleteIdResponse.meterIndex;
	m_doc.AddMember("MeterIndex", v, alloc);
}

void ProtocolBridgeJson::encodeDeleteAllResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	DeleteAllResponse deleteAllResponse = getDeleteAllResponse();

	v = deleteAllResponse.actionResult;
	m_doc.AddMember("ActionResult", v, alloc);
}

void ProtocolBridgeJson::encodeSetNodeTimeoutResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	SetNodeTimeoutResponse setNodeTimeoutResponse = getSetNodeTimeoutResponse();

	v = setNodeTimeoutResponse.requestValue;
	m_doc.AddMember("RequestValue", v, alloc);

	v = setNodeTimeoutResponse.actionResult;
	m_doc.AddMember("ActionResult", v, alloc);
}

void ProtocolBridgeJson::encodeSetAnyPacketModeResponse(
	rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v
) {
	SetAnyPacketModeResponse setAnyPacketModeResponse = getSetAnyPacketModeResponse();

	v = setAnyPacketModeResponse.requestValue;
	m_doc.AddMember("RequestValue", v, alloc);

	v = setAnyPacketModeResponse.actionResult;
	m_doc.AddMember("ActionResult", v, alloc);
}



std::string ProtocolBridgeJson::encodeResponse(const std::string& errStr)
{
	using namespace rapidjson;
	using namespace std::chrono;

	m_doc.RemoveAllMembers();

	addResponseJsonPrio1Params(*this);
	addResponseJsonPrio2Params(*this);
	
	Document::AllocatorType& alloc = m_doc.GetAllocator();
	rapidjson::Value v;

	Cmd cmd = getCmd();
	switch ( cmd ) {
		case Cmd::SYSINFO:
			encodeSysinfoResponse(alloc, v);
			break;

		case Cmd::GET_NEW_VISIBLE:
			encodeGetNewVisibleResponse(alloc, v);
			break;

		case Cmd::GET_NEW_INVISIBLE:
			encodeGetNewInvisibleResponse(alloc, v);
			break;
		
		case Cmd::GET_ALL_VISIBLE:
			encodeGetAllVisibleResponse(alloc, v);
			break;

		case Cmd::GET_NEW_DATA_INFO:
			encodeGetNewDataInfoResponse(alloc, v);
			break;

		case Cmd::GET_COMPACT_PACKET:
			encodeGetCompactPacketResponse(alloc, v);
			break;

		case Cmd::GET_FULL_PACKET:
			encodeGetFullPacketResponse(alloc, v);
			break;

		case Cmd::GET_VISIBLE_CONFIRMATION:
			encodeGetVisibleConfirmationResponse(alloc, v);
			break;

		case Cmd::GET_INVISIBLE_CONFIRMATION:
			encodeGetInvisibleConfirmationResponse(alloc, v);
			break;

		case Cmd::SLEEP_NOW:
			encodeSleepNowResponse(alloc, v);
			break;

		case Cmd::DELETE_ID:
			encodeDeleteIdResponse(alloc, v);
			break;

		case Cmd::DELETE_ALL:
			encodeDeleteAllResponse(alloc, v);
			break;

		case Cmd::SET_NODE_TIMEOUT:
			encodeSetNodeTimeoutResponse(alloc, v);
			break;

		case Cmd::SET_ANY_PACKET_MODE:
			encodeSetAnyPacketModeResponse(alloc, v);
			break;

		default:
			THROW_EX(std::logic_error, "Invalid command: " << PAR((uint8_t)cmd));
	}

	//TODO time from response or transaction?
	time_t tt = system_clock::to_time_t(system_clock::now());
	std::tm* timeinfo = localtime(&tt);
	timeinfo = localtime(&tt);
	std::tm timeStr = *timeinfo;
	char buf[80];
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &timeStr);

	v.SetString(buf, alloc);
	m_doc.AddMember("Time", v, alloc);

	m_statusJ = errStr;
	return encodeResponseJsonFinal(*this);
}
