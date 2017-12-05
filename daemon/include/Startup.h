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

/// \class Startup
/// \brief start application
/// \details
/// Auxiliary class wrapping application init stuff to simplify main function
class Startup
{
public:
  Startup();
  Startup(const Startup&) = delete;
  virtual ~Startup();

  /// \brief start application
  /// \param [in] argc passed number from main()
  /// \param [in] argv passed pointer from main()
  /// \return value -1 in case of initialization error else 0
  /// \details
  /// Auxiliary class wrapping application init stuff to simplify main function
  int run(int argc, char** argv);
};
