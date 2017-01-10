#include "arguments.h"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Utils::Arguments::Arguments(std::string description)
    : m_description(std::move(description))
{
}

void Utils::Arguments::addOption(char key, std::string name, std::string help)
{
  m_options.emplace_back(
      Option{std::move(name), std::move(help), std::string(), key});
}

void Utils::Arguments::addRequiredArgument(std::string name, std::string help)
{
  m_required.emplace_back(Position{std::move(name), std::move(help)});
}

void Utils::Arguments::printHelp(const std::string& arg0) const
{
  using std::cerr;

  std::string name(arg0.substr(arg0.find_last_of('/') + 1));

  cerr << "usage: " << name << " [OPTIONS]";
  for (auto pos = m_required.begin(); pos != m_required.end(); ++pos)
    cerr << ' ' << pos->name;
  cerr << "\n\n" << m_description << "\n\n";
  cerr << "arguments:\n";
  for (auto pos = m_required.begin(); pos != m_required.end(); ++pos) {
    cerr << "  " << std::left << std::setw(20) << pos->name << pos->help
         << '\n';
  }
  cerr << "options:\n";
  for (auto opt = m_options.begin(); opt != m_options.end(); ++opt) {
    std::string desc;
    if (opt->abbreviation != 0) {
      desc += '-';
      desc += opt->abbreviation;
      desc += ',';
    }
    desc += "--";
    desc += opt->name;
    cerr << "  " << std::left << std::setw(20) << desc << opt->help;
    if (!opt->value.empty()) {
      cerr << " (default=" << opt->value << ')';
    }
    cerr << '\n';
  }
  cerr.flush();
}

bool Utils::Arguments::parse(int argc, char const* argv[])
{
  if (static_cast<std::size_t>(argc) < (m_required.size() + 1)) {
    printHelp(argv[0]);
    return true;
  }

  // separated optional flags and positional arguments
  m_args.clear();
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);

    if ((arg.find("--") == 0) && (2 < arg.size())) {
      // long optional argument
      DEBUG("long option: ", arg, ' ', argv[i + 1]);
      Option* opt = find(arg.substr(2), 0);
      if (!opt) {
        std::cerr << "unknown long option: '" << arg << "'\n";
        return true;
      }
      if ((i + 1) < argc)
        opt->value = argv[++i];
    } else if ((arg.find("-") == 0) && (1 < arg.size())) {
      DEBUG("short option: ", arg, ' ', argv[i + 1]);
      // short optional argument
      Option* opt = find("", arg[1]);
      if (!opt) {
        std::cerr << "unknown short option: '" << arg << "'\n";
        return true;
      }
      if ((i + 1) < argc)
        opt->value = argv[++i];
    } else {
      DEBUG("positional argument: ", arg);
      // positional argument
      m_args.emplace_back(std::move(arg));
    }
  }

  if (m_args.size() < m_required.size()) {
    std::cerr << "not enough arguments: " << m_args.size() << " < "
              << m_required.size() << "\n";
    return true;
  }
  return false;
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

const Utils::Arguments::Option* Utils::Arguments::find(const std::string& name,
                                                       char abbreviation) const
{
  for (auto opt = m_options.begin(); opt != m_options.end(); ++opt) {
    if ((opt->name == name) || (opt->abbreviation == abbreviation))
      return &(*opt);
  }
  return NULL;
}
