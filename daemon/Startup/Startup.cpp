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

#include "Startup.h"
#include "VersionInfo.h"

#include "DaemonController.h"
#include "IqrfLogging.h"

#include <signal.h>
#include <chrono>
#include <thread>

#include "PlatformDep.h"

#ifdef WIN
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

DaemonController& msgCtrl = DaemonController::getController();

//catches CTRL-C to stop main loop
void SignalHandler(int signal)
{
  switch (signal)
  {
  case SIGINT:
  case SIGTERM:
  case SIGABRT:
#ifndef WIN
  case SIGQUIT:
  case SIGKILL:
  case SIGSTOP:
  case SIGHUP:
#endif
    msgCtrl.exit();

  default:
    // ...
    break;
  }
}

#ifdef WIN
/// \brief Handler for close signals on Windows.
///
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
  if (dwCtrlType == CTRL_CLOSE_EVENT)
  {
    msgCtrl.exit();
    std::this_thread::sleep_for(std::chrono::milliseconds(10000)); //Win waits ~10 sec and then exits anyway
    return TRUE;
  }
  return FALSE;
}
#endif

Startup::Startup()
{
}

Startup::~Startup()
{
}

int Startup::run(int argc, char** argv)
{
  if (argc == 2 && argv[1] == std::string("version")) {
    std::cout << DAEMON_VERSION << " " << BUILD_TIMESTAMP << std::endl;
    return 0;
  }

  std::string configFile;

  if (SIG_ERR == signal(SIGINT, SignalHandler)) {
    std::cerr << std::endl << "Could not set control handler for SIGINT";
    return -1;
  }

  if (SIG_ERR == signal(SIGTERM, SignalHandler)) {
    std::cerr << std::endl << "Could not set control handler for SIGTERM";
    return -1;
  }
  if (SIG_ERR == signal(SIGABRT, SignalHandler)) {
    std::cerr << std::endl << "Could not set control handler for SIGABRT";
    return -1;
  }
#ifndef WIN
  if (SIG_ERR == signal(SIGQUIT, SignalHandler)) {
    std::cerr << std::endl << "Could not set control handler for SIGQUIT";
    return -1;
  }
#else
  if (SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE) == 0) {
    std::cerr << std::endl << "SetConsoleCtrlHandler failed: GetLastError: " << GetLastError();
    return -1;
  }
#endif

  if (argc < 2) {
    std::cerr << std::endl << "Usage" << std::endl;
    std::cerr << "  iqrf_startup <config file>" << std::endl << std::endl;
    std::cerr << "  iqrf_startup version" << std::endl << std::endl;
    std::cerr << "Example" << std::endl;
    std::cerr << "  iqrf_startup config.json" << std::endl;
    return (-1);
  }
  else {
    configFile = argv[1];
  }

  std::cout << std::endl << argv[0] << std::endl;
  std::cout << 
    "============================================================================" << std::endl <<
    PAR(DAEMON_VERSION) << PAR(BUILD_TIMESTAMP) << std::endl <<
    "============================================================================" << std::endl;

  msgCtrl.run(configFile);

  std::cout << std::endl << argv[0] << " finished";

  return 0;
}
