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

#include <memory>

class DpaTransaction;

/// \class IDpaMessageForwarding
/// \brief IDpaMessageForwarding interface
/// \details
/// Provides interface for forwarding DPA transaction.
/// All DPA messages handled by the transactions are forwarded according implementation
class IDpaMessageForwarding
{
public:
  /// \brief Get DPA transaction forwarding object
  /// \param [in] forwarded DPA transaction to be forwarded
  /// \return Another DPA transaction wrapper
  /// \details
  /// The transaction to be forwarded is wrapped to another transaction object implementing forwrding of flowing message
  virtual std::unique_ptr<DpaTransaction> getDpaTransactionForward(DpaTransaction* forwarded) = 0;

  inline virtual ~IDpaMessageForwarding() {};
};
