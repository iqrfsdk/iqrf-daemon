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

#include "ObjectFactory.h"
#include "DpaTask.h"
#include <memory>
#include <string>

/// Configuration category identification string
static const std::string CAT_CONF_STR("conf");
/// DPA category identification sting
static const std::string CAT_DPA_STR("dpa");

/// \class ISerializer
/// \brief ISerializer interface
class ISerializer
{
public:
  // component
  virtual const std::string& getName() const = 0;

  /// \brief Get category identification from request
  /// \param [in] request incoming request to be examined
  /// \return category string
  /// \details
  /// Extracts a category identification from incoming request.
  /// It allows to identify appropriate parsing method to be used in next processing.
  virtual std::string parseCategory(const std::string& request) = 0;

  /// \brief Parse DPA request
  /// \param [in] request incoming DPA request
  /// \return DPA task created from incomming request
  /// \details
  /// It expects incoming DPA request (detected by parseCategory). It parses DPA request and create DpaTask.
  /// DpaTask may be empty in case of error.
  virtual std::unique_ptr<DpaTask> parseRequest(const std::string& request) = 0;

  /// \brief Parse confiquration request
  /// \param [in] request configuration request
  /// \return string with configuration
  /// \details
  /// It expects incoming configuration request (detected by parseCategory). It parses the request and return configuration string.
  /// The only switch mode forwarding | operational | service is supported now.
  /// Returned string may be empty in case of error.
  virtual std::string parseConfig(const std::string& request) = 0;
  
  /// \brief Encode confiquration response
  /// \param [in] request original configuration request
  /// \param [in] response string with response phrase
  /// \return string with configuration response
  /// \details
  /// Encode configuration response based on original configuration request and passed response phrase.
  virtual std::string encodeConfig(const std::string& request, const std::string& response) = 0;
  
  /// \brief Get last error string
  /// \return error string
  /// \details
  /// Serializer sets last error string as the result of last serialization.
  virtual std::string getLastError() const = 0;

  /// \brief Encode Asynchronous DPA message
  /// \param [in] dpaMessage message to be encoded
  /// \return serialized message
  /// \details
  /// Passed asynchronous DPA message is serialized in an appropriate form.
  virtual std::string encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const = 0;

  virtual ~ISerializer() {}
};
