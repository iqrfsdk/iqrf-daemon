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

#pragma once

#include "DpaTask.h"
#include "JsonSerializer.h"
#include <chrono>

class ProtocolBridge : public DpaTask {
public:
	enum class Cmd {
		SYSINFO,
		GET_NEW_VISIBLE,
		GET_NEW_INVISIBLE,
		GET_ALL_VISIBLE,
		GET_NEW_DATA_INFO,
		GET_COMPACT_PACKET,
		GET_FULL_PACKET,
		GET_VISIBLE_CONFIRMATION,
		GET_INVISIBLE_CONFIRMATION,
		SLEEP_NOW,
		DELETE_ID,
		DELETE_ALL,
		SET_SLEEP_INTERVAL,
		SET_WM_TIMEOUT,
		SET_WM_ALL_READED_TIMEOUT,
		SET_NODE_TIMEOUT,
		SET_ANY_PACKET_MODE
	};

	enum class FrcCmd {
		ALIVE,
		STAT
	};

	static const std::string PRF_NAME;

	ProtocolBridge();
	virtual ~ProtocolBridge();

	// length of bitmap in GET VISIBLE CONFIRMATION and GET INVISIBLE CONFIRMATION
	static const int CONFIRMATION_BITMAP_LEN = 10;

	// commands
	void commandSysinfo();
	void commandGetNewVisible();
	void commandGetNewInvisible();
	void commandGetAllVisible();
	void commandGetNewDataInfo();
	void commandGetCompactPacket(uint16_t meterIndex);
	void commandGetFullPacket(uint16_t meterIndex);
	void commandGetVisibleConfirmation(uint8_t index, uint8_t map[]);
	void commandGetInvisibleConfirmation(uint8_t index, uint8_t map[]);
	void commandSleepNow(uint16_t sleepTime);
	void commandDeleteId(uint16_t meterIndex);
	void commandDeleteAll();
	void commandSetSleepInterval(uint16_t sleepInterval);
	void commandSetWmTimeout(uint16_t wmTimeout);
	void commandSetWmAllReadedTimeout(uint16_t wmTimeout);
	void commandSetNodeTimeout(uint16_t nodeTimeout);
	void commandSetAnyPacketMode(bool enable);

	void parseResponse(const DpaMessage& response) override;
	void parseCommand(const std::string& command) override;
	const std::string& encodeCommand() const override;

	static const int SYSINFO_SERIAL_NUMBER_LEN = 12;
	static const int SYSINFO_FIRMWARE_VERSION_LEN = 5;

	// parsed response data of SYSINFO command 
	struct SysinfoResponse {
		uint8_t serialNumber[SYSINFO_SERIAL_NUMBER_LEN];
		uint16_t visibleMetersNum;
		uint16_t readedMetersNum;
		double temperature;
		double powerVoltage;
		uint8_t firmwareVersion[SYSINFO_FIRMWARE_VERSION_LEN];
		uint16_t sleepCount;
		uint16_t wmbTimeout;
		uint16_t wmbAllReaedTimeout;
		uint16_t nodeTimeout;
		bool anyPacketNode;
		uint8_t countToInvisible;
		uint8_t checksum;
	};


	static const int VISIBLE_METERS_BITMAP_LEN = 38;

	// parsed response data of GET NEW VISIBLE command
	struct NewVisibleMetersResponse {
		uint16_t metersNum;
		uint8_t bitmap[VISIBLE_METERS_BITMAP_LEN];
	};


	static const int INVISIBLE_METERS_BITMAP_LEN = 38;

	// parsed response data of GET NEW INVISIBLE command 
	struct NewInvisibleMetersResponse {
		uint16_t metersNum;
		uint8_t bitmap[INVISIBLE_METERS_BITMAP_LEN];
	};


	static const int ALL_VISIBLE_METERS_BITMAP_LEN = 38;

	// parsed response data of GET ALL VISIBLE command 
	struct AllVisibleMetersResponse {
		uint16_t metersNum;
		uint8_t bitmap[ALL_VISIBLE_METERS_BITMAP_LEN];
	};


	static const int NEW_DATA_INFO_BITMAP_LEN = 38;

	// parsed response data of GET NEW DATA INFO command 
	struct NewDataInfoResponse {
		uint16_t metersNum;
		uint8_t bitmap[NEW_DATA_INFO_BITMAP_LEN];
	};

	// maximal length of wmbus frame
	static const int VMBUS_MAX_FRAME_LEN = 45;

	// parsed response data of GET COMPACT PACKET command
	struct CompactPacketResponse {
		uint16_t meterIndex;
		uint8_t vmbusMsg[VMBUS_MAX_FRAME_LEN];
		uint8_t msgLen;
		uint8_t rssi;
		uint8_t crc;
		uint8_t afterReceiptWmbusChecksum;
		uint8_t countedWmbusChecksum;
	};

	// parsed response data of GET FULL PACKET command
	struct FullPacketResponse {
		uint16_t meterIndex;
		uint8_t vmbusMsg[VMBUS_MAX_FRAME_LEN];
		uint8_t msgLen;
		uint8_t rssi;
		uint8_t crc;
		uint8_t afterReceiptWmbusChecksum;
		uint8_t countedWmbusChecksum;
	};

	// parsed response data of GET VISIBLE CONFIRMATION command 
	struct VisibleConfirmationResponse {
		uint8_t confirmedNum;
		bool actionResult;
	};

	// parsed response data of GET INVISIBLE CONFIRMATION command 
	struct InvisibleConfirmationResponse {
		uint8_t confirmedNum;
		bool actionResult;
	};

	// parsed response data of SLEEP NOW command
	struct SleepNowResponse {
		uint16_t requestValue;
	};

	// parsed response data of DELETE ID command
	struct DeleteIdResponse {
		uint16_t meterIndex;
	};

	// parsed response data of DELETE ALL command
	struct DeleteAllResponse {
		bool actionResult;
	};

	// parsed response data of SET NODE TIMEOUT command
	struct SetNodeTimeoutResponse {
		uint16_t requestValue;
		bool actionResult;
	};

	// parsed response data of SET ANY PACKET MODE command
	struct SetAnyPacketModeResponse {
		uint16_t requestValue;
		bool actionResult;
	};


	// PARSED RESPONSES GETTERS

	SysinfoResponse getSysinfoResponse() const { return sysinfoResponse; };

	NewVisibleMetersResponse getNewVisibleMetersResponse() const { 
		return newVisibleMetersResponse; 
	};

	NewInvisibleMetersResponse getNewInvisibleMetersResponse() const { 
		return newInvisibleMetersResponse; 
	};

	AllVisibleMetersResponse getAllVisibleMetersResponse() const { 
		return allVisibleMetersResponse; 
	};

	NewDataInfoResponse getNewDataInfoResponse() const { 
		return newDataInfoResponse; 
	};

	CompactPacketResponse getCompactPacketResponse() const {
		return compactPacketResponse;
	};

	FullPacketResponse getFullPacketResponse() const {
		return fullPacketResponse;
	};

	VisibleConfirmationResponse getVisibleConfirmationResponse() const {
		return visibleConfirmationResponse; 
	};

	InvisibleConfirmationResponse getInvisibleConfirmationResponse() const { 
		return invisibleConfirmationResponse; 
	};

	SleepNowResponse getSleepNowResponse() const {
		return sleepNowResponse;
	};

	DeleteIdResponse getDeleteIdResponse() const {
		return deleteIdReponse;
	};

	DeleteAllResponse getDeleteAllResponse() const {
		return deleteAllResponse;
	};

	SetNodeTimeoutResponse getSetNodeTimeoutResponse() const {
		return setNodeTimeoutResponse;
	};

	SetAnyPacketModeResponse getSetAnyPacketModeResponse() const {
		return setAnyPacketModeResponse;
	};

	Cmd getCmd() const;


private:
	void setCmd(Cmd cmd);

	Cmd m_cmd = Cmd::SYSINFO;

	// parsed responses
	SysinfoResponse sysinfoResponse;
	NewVisibleMetersResponse newVisibleMetersResponse;
	NewInvisibleMetersResponse newInvisibleMetersResponse;
	AllVisibleMetersResponse allVisibleMetersResponse;
	NewDataInfoResponse newDataInfoResponse;
	CompactPacketResponse compactPacketResponse;
	FullPacketResponse fullPacketResponse;
	VisibleConfirmationResponse visibleConfirmationResponse;
	InvisibleConfirmationResponse invisibleConfirmationResponse;
	SleepNowResponse sleepNowResponse;
	DeleteIdResponse deleteIdReponse;
	DeleteAllResponse deleteAllResponse;
	SetNodeTimeoutResponse setNodeTimeoutResponse;
	SetAnyPacketModeResponse setAnyPacketModeResponse;


	// helper methods for response parsing
	void parseSysinfoResponse(const uint8_t* pData);
	void parseGetNewVisibleResponse(const uint8_t* pData);
	void parseGetNewInvisibleResponse(const uint8_t* pData);
	void parseGetAllVisibleResponse(const uint8_t* pData);
	void parseGetNewDataInfoResponse(const uint8_t* pData);
	void parseGetCompactPacketResponse(const uint8_t* pData);
	void parseGetFullPacketResponse(const uint8_t* pData);
	void parseGetVisibleConfirmationResponse(const uint8_t* pData);
	void parseGetInvisibleConfirmationResponse(const uint8_t* pData);
	void parseSleepNowResponse(const uint8_t* pData);
	void parseDeleteIdResponse(const uint8_t* pData);
	void parseDeleteAllResponse(const uint8_t* pData);
	/*
	void parseSetSleepIntervalResponse(const uint8_t* pData);
	void parseSetWmTimeoutResponse(const uint8_t* pData);
	void parseSetWmAllReadedTimeoutResponse(const uint8_t* pData);
	*/
	void parseSetNodeTimeoutResponse(const uint8_t* pData);
	void parseSetAnyPacketModeResponse(const uint8_t* pData);
};

class ProtocolBridgeJson : public ProtocolBridge, public PrfCommonJson
{
public:
	ProtocolBridgeJson();
	ProtocolBridgeJson(const ProtocolBridgeJson& o);
	explicit ProtocolBridgeJson(const rapidjson::Value& val);
	void setNadr(uint16_t nadr);
	virtual ~ProtocolBridgeJson() {}
	std::string encodeResponse(const std::string& errStr) override;

private:
	// helper methods for encoding responses
	void encodeSysinfoResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetNewVisibleResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetNewInvisibleResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetAllVisibleResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetNewDataInfoResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetCompactPacketResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetFullPacketResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetVisibleConfirmationResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeGetInvisibleConfirmationResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeSleepNowResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeDeleteIdResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeDeleteAllResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeSetNodeTimeoutResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
	void encodeSetAnyPacketModeResponse(rapidjson::Document::AllocatorType& alloc, rapidjson::Value& v);
};
