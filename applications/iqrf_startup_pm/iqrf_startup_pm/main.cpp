#include "initModules.h"
#include "Startup.h"
#include "PlatformDep.h"

#if defined(WIN) && defined(_DEBUG)
#include <crtdbg.h>
#endif

int main(int argc, char** argv)
{
#if defined(WIN) && defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
  STATIC_INIT
  Startup startup;
  startup.run(argc, argv);
}
