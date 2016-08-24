#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "mechanics/device.h"

void print_help(const std::string& name)
{
  using std::cout;

  cout << "usage: " << name << " CONFIG_PATH\n\n";
  cout << "read and show device configuration" << std::endl;
}

int main(int argc, char const* argv[])
{
  const std::string arg0(argv[0]);
  const std::string name(arg0.substr(arg0.find_last_of('/') + 1));

  if (argc != 2) {
    print_help(name);
    return EXIT_FAILURE;
  }

  Mechanics::Device::fromFile(argv[1]).print(std::cout);

  return EXIT_SUCCESS;
}
