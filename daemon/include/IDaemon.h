/*
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

#include "DpaTransaction.h"
#include <string>

typedef std::basic_string<unsigned char> ustring;
/// Asynchronous DPA message handler functional type
typedef std::function<void(const DpaMessage& dpaMessage)> AsyncMessageHandlerFunc;

/// Forward declaration of IScheduler interface
class IScheduler;

/// \class IDaemon
/// \brief IDaemon interface
class IDaemon
{
public:
  virtual ~IDaemon() {};

  /// \brief Execute DPA transaction
  /// \param [in]     dpaTransaction Transaction to be executed
  /// \details
  /// The transaction consists from DPA requeste sent to coordinator. It is finished by DPA response or timeout
  virtual void executeDpaTransaction(DpaTransaction& dpaTransaction) = 0;

  /// \brief Register Asynchronous DPA message handler
  /// \param [in] clientId client identification registering handler function
  /// \param [in] fun handler function
  /// \details
  /// Whenever an asynchronous DPA message is received its passed to the handler function. It is possible to register 
  /// more handlers for different clients distinguished via client identifications.
  /// All registered handlers are invoked to handle the message, however the order is not quaranteed.
  /// Repeated registration with the same client identification replaces previously registered handler
  virtual void registerAsyncMessageHandler(const std::string& clientId, AsyncMessageHandlerFunc fun) = 0;

  /// \brief Unregister Asynchronous DPA message handler
  /// \param [in] clientId client identification
  /// \details
  /// If the handler is not required anymore, it is possible to unregister via this method.
  virtual void unregisterAsyncMessageHandler(const std::string& clientId) = 0;

  /// \brief Get IScheduler implementation
  /// \return IScheduler implementation
  /// \details
  /// Provides an instance of IScheduler implementation.
  virtual IScheduler* getScheduler() = 0;

  /// \brief Perform mode switch command
  /// \param [in] cmd command to switch mode
  /// \return result of the command
  /// \details
  /// Switches communication mode to oparational, forwarding or service according the command
  /// Supported command strings are: "operational" | "service" | "forwarding"
  virtual std::string doCommand(const std::string& cmd) = 0;

  /// \brief Get IQRF coordination identification
  /// \details
  /// \return Module ID
  /// Module ID is taken from the coordinator at the initialization phase, e.g. "8100528a"
  virtual const std::string& getModuleId() = 0;

  /// \brief Get IQRF coordination OS version
  /// \return OS version
  /// \details
  /// OS version is taken from the coordinator at the initialization phase, e.g. "3.08D"
  virtual const std::string& getOsVersion() = 0;

  /// \brief Get IQRF coordination TR type
  /// \return TR type
  /// \details
  /// TR type is taken from the coordinator at the initialization phase, e.g. "DCTR-72D"
  virtual const std::string& getTrType() = 0;

  /// \brief Get IQRF coordination MCU type
  /// \return MCU type
  /// \details
  /// MCU type is taken from the coordinator at the initialization phase, e.g. "PIC16F1938"
  virtual const std::string& getMcuType() = 0;

  /// \brief Get IQRF coordination OS build
  /// \return OS build
  /// \details
  /// OS build is taken from the coordinator at the initialization phase, e.g. "0879"
  virtual const std::string& getOsBuild() = 0;

  /// \brief Get iqrf-daemon Version
  /// \return Version
  /// \details
  /// Version is hardcoded during build, e.g. "v1.0.0"
  virtual const std::string& getDaemonVersion() = 0;

  /// \brief Get iqrf-daemon build Timestamp
  /// \return Timestamp
  /// \details
  /// Timestamp is hardcoded during build, e.g. "Fri 06/23/2017 9:53:37.74"
  virtual const std::string& getDaemonVersionBuild() = 0;
};
