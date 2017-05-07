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

#include "ObjectFactory.h"
#include "DpaTask.h"
#include <memory>
#include <string>

const std::string CAT_CONF_STR("conf");
const std::string CAT_DPA_STR("dpa");

class ISerializer
{
public:
  // component
  virtual const std::string& getName() const = 0;

  // interface
  virtual std::string parseCategory(const std::string& request) = 0;
  virtual std::unique_ptr<DpaTask> parseRequest(const std::string& request) = 0;
  virtual std::string parseConfig(const std::string& request) = 0;
  //virtual std::string parseSched(const std::string& request);
  //virtual std::string parseStat(const std::string& request);
  virtual std::string getLastError() const = 0;

  virtual ~ISerializer() {}
};
