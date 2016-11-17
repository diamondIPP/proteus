/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include <iostream>
#include <string>

#include "mechanics/device.h"
#include "utils/logger.h"

int main(int argc, char const* argv[])
{
  if (argc != 2) {
    std::string arg0(argv[0]);
    std::string name(arg0.substr(arg0.find_last_of('/') + 1));
    std::cerr << "usage: " << name << " DEVICE_PATH\n";
    std::cerr << '\n';
    std::cerr << "show device configuration\n";
    std::cerr.flush();
    return EXIT_FAILURE;
  }

  Utils::Logger::setGlobalLevel(Utils::Logger::ERROR);
  Mechanics::Device::fromFile(argv[1]).print(std::cout);

  return EXIT_SUCCESS;
}
