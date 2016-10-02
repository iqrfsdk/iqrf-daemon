#include "MessageHandler.h"
#include "IqrfLogging.h"

TRC_INIT("");

int main(int argc, char** argv)
{
  TRC_ENTER("started");
  std::string iqrf_port_name;
  std::string remote_ip_port("55000");
  std::string local_ip_port("55300");

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

  std::cout << "iqrf_gw started" << std::endl;
  TRC_ENTER(PAR(iqrf_port_name) << PAR(remote_ip_port) << PAR(local_ip_port));

  try {
    MessageHandler msgHndl(iqrf_port_name, remote_ip_port, local_ip_port);
    msgHndl.watchDog();
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  TRC_ENTER("finished");
}
