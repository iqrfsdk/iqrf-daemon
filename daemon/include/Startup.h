#pragma once

class Startup
{
public:
  Startup();
  Startup(const Startup&) = delete;
  virtual ~Startup();
  int run(int argc, char** argv);
};
