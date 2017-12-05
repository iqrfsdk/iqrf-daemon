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

#include "JsonUtils.h"
#include "ProtocolBridge.h"
#include "IService.h"
#include "ISerializer.h"
#include "IMessaging.h"
#include "IScheduler.h"
#include "TaskQueue.h"
#include <string>
#include <chrono>
#include <vector>
#include <list>
#include <memory>
#include <unordered_map>

class IDaemon;

typedef std::basic_string<unsigned char> ustring;

template <typename T>
class ScheduleDpaTask
{
public:
	ScheduleDpaTask() = delete;
	ScheduleDpaTask(const T& dpt, IScheduler* schd)
		:m_scheduler(schd)
		, m_dpa(dpt)
		, m_sync(false)
	{}

	ScheduleDpaTask(const ScheduleDpaTask& other)
		: m_taskHandle(other.m_taskHandle)
		, m_scheduler(other.m_scheduler)
		, m_dpa(other.m_dpa)
		, m_sync((bool)other.m_sync)
	{
	}

	virtual ~ScheduleDpaTask() {};

	bool isSync() const { return m_sync; }
	void setSync(bool val) { m_sync = val; }

	void scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
		const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now())
	{
		removeSchedule(clientId);
		m_taskHandle = m_scheduler->scheduleTaskPeriodic(clientId, task, sec, tp);
	}

	bool isScheduled()
	{
		return m_taskHandle != IScheduler::TASK_HANDLE_INVALID;
	}

	void removeSchedule(const std::string& clientId)
	{
		m_scheduler->removeTask(clientId, m_taskHandle);
		m_taskHandle = IScheduler::TASK_HANDLE_INVALID;
	}

	T& getDpa() { return m_dpa; }
private:
	std::atomic_bool m_sync{ false };
	IScheduler::TaskHandle m_taskHandle = IScheduler::TASK_HANDLE_INVALID;
	IScheduler* m_scheduler;
	T m_dpa;
};

typedef ScheduleDpaTask<ProtocolBridgeJson> ProtocolBridgeSchd;

class ProtocolBridgeClientService : public IService
{
public:
	ProtocolBridgeClientService() = delete;
	ProtocolBridgeClientService(const std::string& name);
	virtual ~ProtocolBridgeClientService();

	void updateConfiguration(const rapidjson::Value& cfg);

	void setDaemon(IDaemon* daemon) override;
	virtual void setSerializer(ISerializer* serializer) override;
	virtual void setMessaging(IMessaging* messaging) override;
	const std::string& getName() const override {
	return m_name;
	}

	void update(const rapidjson::Value& cfg) override;
	void start() override;
	void stop() override;

private:
	void handleMsgFromMessaging(const ustring& msg);
	void handleTaskFromScheduler(const std::string& task);
	
	// data inside FRC STAT response for each Protocol Bridge
	struct BridgeStatusData {
		bool isOn;
		bool isData;
		bool isNewVisible;
		bool isNewInvisible;
	};
	
	// length of manufacturer field of packet header
	static const int MANUFACTURER_LEN = 2;

	// length of address field of packet header
	static const int ADDRESS_LEN = 6;

	// parsed header of a Protocol Bridge packet
	struct PacketHeader {
		uint8_t manufacturer[MANUFACTURER_LEN];
		uint8_t address[ADDRESS_LEN];
	};

	std::map<uint8_t, BridgeStatusData> getActiveBridgesStatusMap();
	std::list<uint8_t> getNewVisibleMetersIndexes(uint8_t bridgeAddress);
	std::list<uint8_t> getNewInvisibleMetersIndexes(uint8_t bridgeAddress);
	std::list<uint8_t> getNewDataMetersIndexes(uint8_t bridgeAddress);
	ProtocolBridge::FullPacketResponse getFullPacketResponse(uint8_t bridgeAddress, uint8_t meterIndex);
	void confirmVisibleMeters(uint8_t bridgeAddress, std::list<uint8_t> newVisibleMetersIndexes);
	void confirmInvisibleMeters(uint8_t bridgeAddress, std::list<uint8_t> newInvisibleMetersIndexes);
	PacketHeader parseFullPacketResponse(ProtocolBridge::FullPacketResponse fullPacketResponse);
	void sendDataIntoAzure(PacketHeader packetHeader, uint8_t data[], int dataLen);
	void sleepProtocolBridge(uint8_t bridgeAddress);
  void timeoutWMBProtocolBridge(uint8_t bridgeAddress);

	void getAndProcessDataFromMeters(const std::string& task);

	std::string m_name;

  // how often to send frc status cmd in sec
  uint16_t m_frcPeriod = 30;

	// sleeping period is this value * HW interval (sec)
	uint16_t m_sleepPeriod = 10;

  // timeout for wmbus module, module is on for this time
  uint16_t m_wmPeriod = 60;

	IMessaging* m_messaging;
	IDaemon* m_daemon;
	ISerializer* m_serializer;

	std::unordered_map<int, ProtocolBridgeSchd> m_watchedProtocolBridges;
};
