#include "MessageHandler.h"
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

class MainException : public std::exception {
public:
  MainException(const std::string& cause)
    :m_cause(cause)
  {}

  //TODO ?
#ifndef WIN32
  virtual const char* what() const noexcept(true)
#else
  virtual const char* what() const
#endif
  {
    return m_cause.c_str();
  }

  virtual ~MainException()
  {}

protected:
  std::string m_cause;
};

TRC_INIT("");

MessageHandler* msgHndl(nullptr);

//-------------------------------------------------
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
    if (nullptr != msgHndl)
      msgHndl->exit();
    break;

  default:
    // ...
    break;
  }
}

#ifdef WIN
//--------------------------------------------------------------------------------------------------
/// \brief Handler for close signals on Windows.
///
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
  if (dwCtrlType == CTRL_CLOSE_EVENT)
  {
    if (nullptr != msgHndl)
      msgHndl->exit();
    std::this_thread::sleep_for(std::chrono::milliseconds(10000)); //Win waits ~10 sec and then exits anyway
    return TRUE;
  }
  return FALSE;
}
#endif

//--------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
#if defined(WIN) && defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
  TRC_ENTER("started");

  std::string iqrf_port_name;
  std::string remote_ip_port("55000");
  std::string local_ip_port("55300");

  try {
    if (SIG_ERR == signal(SIGINT, SignalHandler)) {
      THROW_EX(MainException, "ERROR: Could not set control handler for SIGINT");
    }

    if (SIG_ERR == signal(SIGTERM, SignalHandler)) {
      THROW_EX(MainException, "ERROR: Could not set control handler for SIGTERM");
      return -1;
    }
#ifndef WIN
    if (SIG_ERR == signal(SIGQUIT, SignalHandler)) {
      THROW_EX(MainException, "ERROR: Could not set control handler for SIGQUIT");
    }
#endif
    if (SIG_ERR == signal(SIGABRT, SignalHandler)) {
      THROW_EX(MainException, "ERROR: Could not set control handler for SIGABRT");
    }

#ifdef WIN
    if (SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE) == 0) {
      THROW_EX(MainException, "SetConsoleCtrlHandler failed: GetLastError: " << GetLastError());
    }
#endif

    if (argc < 2) {
      std::cerr << "Usage" << std::endl;
      std::cerr << "  cdc_example <iqrf_port_name> [remote_ip_port] [local_ip_port]" << std::endl << std::endl;
      std::cerr << "Example" << std::endl;
      std::cerr << "  cdc_example COM5" << std::endl;
      std::cerr << "  cdc_example /dev/ttyACM0 55000 55300" << std::endl;
      return (-1);
    }
    else {
      iqrf_port_name = argv[1];
      if (argc > 2) {
        remote_ip_port = argv[2];
      }
      if (argc > 3) {
        local_ip_port = argv[3];
      }
    }
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
    return -1;
  }

  TRC_ENTER(PAR(iqrf_port_name) << PAR(remote_ip_port) << PAR(local_ip_port));

  msgHndl = ant_new MessageHandler(iqrf_port_name, remote_ip_port, local_ip_port);
  msgHndl->watchDog();

  TRC_DBG("deleting msgHndl");
  delete msgHndl;

  TRC_ENTER("finished");
}
