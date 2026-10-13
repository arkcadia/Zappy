#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
class Gui {
  public:
    std::string hostname;
    uint16_t port;

    Gui(int argc, char *argv[]);

  private:
    void validate() const;
};
