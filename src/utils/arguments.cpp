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
  addFlag('h', "help", "print this help text");
}

void Utils::Arguments::addFlag(char key, std::string name, std::string help)
{
  Option opt;
  opt.abbreviation = key;
  opt.name = std::move(name);
  opt.help = std::move(help);
  opt.type = OptionType::kFlag;
  m_options.emplace_back(std::move(opt));
}

void Utils::Arguments::addOptional(char key, std::string name, std::string help)
{
  Option opt;
  opt.abbreviation = key;
  opt.name = std::move(name);
  opt.help = std::move(help);
  opt.type = OptionType::kSingle;
  m_options.emplace_back(std::move(opt));
}

void Utils::Arguments::addRequired(std::string name, std::string help)
{
  Required req;
  req.name = std::move(name);
  req.help = std::move(help);
  m_requireds.emplace_back(std::move(req));
}

std::string Utils::Arguments::Option::description() const
{
  std::string desc;
  if (this->abbreviation != kNoAbbr) {
    desc += '-';
    desc += this->abbreviation;
    desc += ',';
  }
  desc += "--";
  desc += this->name;
  return desc;
}

void Utils::Arguments::printHelp(const std::string& arg0) const
{
  using std::cerr;

  std::string name(arg0.substr(arg0.find_last_of('/') + 1));

  cerr << "usage: " << name << " [options]";
  for (const auto& req : m_requireds) {
    cerr << " " << req.name;
  }
  cerr << "\n\n";
  cerr << m_description << "\n";
  cerr << "\n";
  cerr << "required:\n";
  for (const auto& req : m_requireds) {
    cerr << "  " << std::left << std::setw(17) << req.name;
    cerr << " " << req.help << "\n";
  }
  cerr << "options:\n";
  for (const auto& opt : m_options) {
    cerr << "  " << std::left << std::setw(17) << opt.description();
    cerr << " " << opt.help;
    if (!opt.defaultValue.empty()) {
      cerr << " (default=" << opt.defaultValue << ")";
    }
    cerr << "\n";
  }
  cerr.flush();
}

// always return true to indicate error
static inline bool args_fail(const std::string& msg)
{
  std::cerr << msg;
  std::cerr << "\ntry --help for more information" << std::endl;
  return true;
}

bool Utils::Arguments::parse(int argc, char const* argv[])
{
  size_t numArgs = 0;

  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg.find("-") == 0) {
      DEBUG("arg ", i, " optional ", arg);

      // search for compatible long or short option
      const Option* opt = nullptr;
      if (arg.find("--") == 0) {
        opt = find(arg.substr(2));
      } else {
        opt = find(arg[1]);
      }
      if (!opt)
        return args_fail("unknown option '" + arg + "'");

      // options must only be set once
      if (m_values.count(opt->name) == 1)
        return args_fail("duplicate option '" + arg + "'");

      // process depending on option type
      if (opt->type == OptionType::kSingle) {
          if (argc <= (i + 1))
            return args_fail("option '" + arg + "' requires a parameter");
          m_values[opt->name] = argv[++i];
      } else /* OptionType::kFlag */ {
          m_values[opt->name] = "true";
      }

    } else {
      DEBUG("arg ", i, " required ", arg);

      if (m_requireds.size() < (numArgs + 1))
        return args_fail("too many arguments");
      m_values[m_requireds[numArgs].name] = arg;
      numArgs += 1;
    }
  }

  if (has("help")) {
    printHelp(argv[0]);
    return true;
  }

  if (numArgs < m_requireds.size())
    return args_fail("not enough arguments");

  // add missing default values for optional argument
  for (const auto& opt : m_options) {
    if (!opt.defaultValue.empty() && (m_values.count(opt.name) == 0)) {
      m_values[opt.name] = opt.defaultValue;
    }
  }
  // debug list of available values
  for (const auto& val : m_values)
    DEBUG(val.first, ": ", val.second);
  return false;
}

const Utils::Arguments::Option*
Utils::Arguments::find(const std::string& name) const
{
  for (const auto& opt : m_options) {
    if (opt.name == name)
      return &opt;
  }
  return NULL;
}

const Utils::Arguments::Option* Utils::Arguments::find(char abbreviation) const
{
  for (const auto& opt : m_options) {
    if ((opt.abbreviation != kNoAbbr) && (opt.abbreviation == abbreviation))
      return &opt;
  }
  return NULL;
}
