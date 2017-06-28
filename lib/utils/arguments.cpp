#include "arguments.h"

#include <cassert>
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

void Utils::Arguments::addOption(char key, std::string name, std::string help)
{
  Option opt;
  opt.abbreviation = key;
  opt.name = std::move(name);
  opt.help = std::move(help);
  opt.type = OptionType::kSingle;
  m_options.emplace_back(std::move(opt));
}

void Utils::Arguments::addOptionMulti(char key,
                                      std::string name,
                                      std::string help)
{
  Option opt;
  opt.abbreviation = key;
  opt.name = std::move(name);
  opt.help = std::move(help);
  opt.type = OptionType::kMulti;
  m_options.emplace_back(std::move(opt));
}

void Utils::Arguments::addRequired(std::string name, std::string help)
{
  RequiredArgument arg;
  arg.name = std::move(name);
  arg.help = std::move(help);
  m_requireds.emplace_back(std::move(arg));
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
  for (const auto& arg : m_requireds) {
    cerr << " " << arg.name;
  }
  cerr << "\n\n";
  cerr << m_description << "\n";
  cerr << "\n";
  cerr << "arguments:\n";
  for (const auto& arg : m_requireds) {
    cerr << "  " << std::left << std::setw(17) << arg.name;
    cerr << " " << arg.help << "\n";
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
      DEBUG("arg ", i, " option ", arg);

      // search for compatible long or short option
      const Option* opt = nullptr;
      if (arg.find("--") == 0) {
        opt = find(arg.substr(2));
      } else {
        opt = find(arg[1]);
      }
      if (!opt)
        return args_fail("unknown option '" + arg + "'");

      // options, except the multi option, must only be set once
      if ((opt->type != OptionType::kMulti) && (m_values.count(opt->name) == 1))
        return args_fail("duplicate option '" + arg + "'");

      // process depending on option type
      switch (opt->type) {
      case OptionType::kFlag:
        m_values[opt->name] = "true";
        break;
      case OptionType::kSingle:
        if (argc <= (i + 1))
          return args_fail("option '" + arg + "' requires a parameter");
        m_values[opt->name] = argv[++i];
        break;
      case OptionType::kMulti:
        if (argc <= (i + 1))
          return args_fail("option '" + arg + "' requires a parameter");
        // multi options are stored internally as komma separated string
        if (!m_values[opt->name].empty()) {
          m_values[opt->name] += ',';
        }
        m_values[opt->name] += argv[++i];
        break;
      default:
        assert(false && "The option type uses an undefined value.");
      }

    } else {
      DEBUG("arg ", i, " argument ", arg);

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
