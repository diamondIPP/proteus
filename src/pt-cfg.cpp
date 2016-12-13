/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include <iostream>
#include <string>

#include "mechanics/device.h"
#include "mechanics/geometry.h"
#include "mechanics/noisemask.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

int main(int argc, char const* argv[])
{
  if (argc != 2) {
    std::string arg0(argv[0]);
    std::string name(arg0.substr(arg0.find_last_of('/') + 1));
    std::cerr << "usage: " << name << " CONFIG\n";
    std::cerr << '\n';
    std::cerr << "show device/geometry/mask configuration\n";
    std::cerr.flush();
    return EXIT_FAILURE;
  }
  std::string path(argv[1]);

  Utils::Logger::setGlobalLevel(Utils::Logger::Level::Error);
  // try different types of configurations
  try {
    Mechanics::Device::fromFile(path).print(std::cout);
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    INFO("not a device config: ", e.what());
  }
  try {
    Mechanics::Geometry::fromFile(path).print(std::cout);
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    INFO("not a geometry config: ", e.what());
  }
  try {
    Mechanics::NoiseMask::fromFile(path).print(std::cout);
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    INFO("not a mask file: ", e.what());
  }
  // reached only if nothing works
  std::cerr << '\'' << path << "' is not a valid configuration file\n";
  return EXIT_FAILURE;
}
