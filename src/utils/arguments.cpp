#include "arguments.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Utils::Arguments::Arguments(const char* description)
    : m_description(description)
{
  // default optional arguments
  addOption('d', "device", "device configuration file", "configs/device.toml");
  addOption('c', "config", "analysis configuration file",
            "configs/analysis.toml");
  addOption('s', "skip_events", "skip the first n events", 0);
  addOption('n', "num_events", "number of events to process", -1);
}

void Utils::Arguments::printHelp(const std::string& arg0) const
{
  using std::cerr;

  std::string name(arg0.substr(arg0.find_last_of('/') + 1));

  cerr << "usage: " << name << " [OPTIONS] INPUT OUTPUT_PREFIX\n";
  cerr << '\n';
  cerr << m_description << '\n';
  cerr << '\n';
  cerr << "options:\n";
  for (auto opt = m_options.begin(); opt != m_options.end(); ++opt) {
    cerr << "  ";
    if (opt->abbreviation != 0)
      cerr << "-" << opt->abbreviation;
    cerr << ",--" << opt->name;
    cerr << " \t" << opt->help;
    if (!opt->value.empty()) {
      cerr << " (default=" << opt->value << ')';
    }
    cerr << '\n';
  }
  cerr.flush();
}

Utils::Arguments::Option* Utils::Arguments::find(const std::string& name,
                                                   char abbreviation)
{
  for (auto opt = m_options.begin(); opt != m_options.end(); ++opt) {
    if ((opt->name == name) || (opt->abbreviation == abbreviation))
      return &(*opt);
  }
  return NULL;
}

const Utils::Arguments::Option*
Utils::Arguments::find(const std::string& name, char abbreviation) const
{
  for (auto opt = m_options.begin(); opt != m_options.end(); ++opt) {
    if ((opt->name == name) || (opt->abbreviation == abbreviation))
      return &(*opt);
  }
  return NULL;
}

bool Utils::Arguments::parse(int argc, char const* argv[])
{
  if (argc < 3) {
    printHelp(argv[0]);
    return true;
  }

  // separated optional flags and positional arguments
  m_positionals.clear();
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);

    if ((arg.find("--") == 0) && (2 < arg.size())) {
      // long optional argument
      DEBUG("long option: ", arg, ' ', argv[i + 1]);
      Option* opt = find(arg.substr(2), 0);
      if (!opt) {
        std::cerr << "unknown long option '" << arg << "'\n\n";
        return true;
      }
      if ((i + 1) < argc)
        opt->value = argv[++i];
    } else if ((arg.find("-") == 0) && (1 < arg.size())) {
      DEBUG("short option: ", arg, ' ', argv[i + 1]);
      // short optional argument
      Option* opt = find("", arg[1]);
      if (!opt) {
        std::cerr << "unknown short option '" << arg << "'\n\n";
        return true;
      }
      if ((i + 1) < argc)
        opt->value = argv[++i];
    } else {
      DEBUG("positional argument: ", arg);
      // positional argument
      m_positionals.emplace_back(std::move(arg));
    }
  }

  if (m_positionals.size() != 2) {
    std::cerr << "incorrect number of arguments\n\n";
    return true;
  }
  m_input = m_positionals[0];
  m_outputPrefix = m_positionals[1];
  return false;
}

std::string Utils::Arguments::device() const
{
  return get<std::string>("device");
}

std::string Utils::Arguments::config() const
{
  return get<std::string>("config");
}

std::string Utils::Arguments::makeOutput(const std::string& name) const
{
  std::string out = outputPrefix();
  out += '-';
  out += name;
  return out;
}
