#include "arguments.h"

#include <cstring>
#include <iostream>
#include <vector>

Utils::Arguments::Arguments(const char* description)
    : m_description(description)
{
  // default values
  m_opts["-d"] = "configs/device.toml";
  m_opts["-c"] = "configs/analysis.toml";
}

void Utils::Arguments::printHelp(const std::string& arg0)
{
  using std::cerr;

  std::string name(arg0.substr(arg0.find_last_of('/') + 1));

  cerr << "usage: " << name << " [OPTIONS] INPUT OUTPUT_PREFIX\n";
  cerr << '\n';
  cerr << m_description << '\n';
  cerr << '\n';
  cerr << "options:\n";
  cerr << "  -d PATH  device description (default=" << device() << ") \n";
  cerr << "  -c PATH  configuration (default=" << config() << ")\n";
  cerr.flush();
}

bool Utils::Arguments::parse(int argc, char const* argv[])
{
  if (argc < 3) {
    printHelp(argv[0]);
    return true;
  }

  // separated optional flags and positional arguments
  std::vector<std::string> positional;
  for (int i = 1; i < argc; ++i) {
    auto opt = m_opts.find(argv[i]);
    if (opt != m_opts.end()) {
      // known optional argument
      if ((i + 1) < argc) {
        opt->second = argv[++i];
      }
    } else if (argv[i][0] == '-') {
      // unkown optional argument
      std::cerr << "unknown argument: " << argv[i] << "\n\n";
      return true;
    } else {
      // positional argument
      positional.emplace_back(argv[i]);
    }
  }

  if (positional.size() != 2) {
    std::cerr << "incorrect number of arguments\n\n";
    return true;
  }
  m_input = positional[0];
  m_outputPrefix = positional[1];
  return false;
}

const std::string& Utils::Arguments::device() const { return m_opts.at("-d"); }
const std::string& Utils::Arguments::config() const { return m_opts.at("-c"); }

std::string Utils::Arguments::makeOutput(const std::string& name) const
{
  std::string out = outputPrefix();
  out += '-';
  out += name;
  return out;
}
