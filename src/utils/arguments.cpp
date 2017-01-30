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

void Utils::Arguments::addFlag(char key, std::string name, std::string help)
{
  m_options.emplace_back(Option{std::move(name), std::move(help), std::string(),
                                key, OptionType::kFlag});
}

void Utils::Arguments::addOptional(char key, std::string name, std::string help)
{
  m_options.emplace_back(Option{std::move(name), std::move(help), std::string(),
                                key, OptionType::kSingle});
}

void Utils::Arguments::addRequired(std::string name, std::string help)
{
  m_requireds.emplace_back(Required{std::move(name), std::move(help)});
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
  cerr << "optional:\n";
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

bool Utils::Arguments::parse(int argc, char const* argv[])
{
  using std::cerr;
  using std::endl;

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
      if (!opt) {
        cerr << "unknown option '" << arg << '\'' << endl;
        return true;
      }
      // options must only be set once
      if (m_values.count(opt->name) == 1) {
        cerr << "option '" << opt->description() << "' is already set" << endl;
        return true;
      }

      // process depending on option type
      if (opt->type == OptionType::kSingle) {
        if (argc <= (i + 1)) {
          cerr << "option '" << arg << "' requires a parameter" << endl;
          return true;
        }
        m_values[opt->name] = argv[++i];
      } else /* OptionType::kFlag */ {
        m_values[opt->name] = "true";
      }

    } else {
      DEBUG("arg ", i, " required ", arg);

      if (m_requireds.size() < (numArgs + 1)) {
        cerr << "too many arguments" << endl;
        return true;
      }
      m_values[m_requireds[numArgs].name] = arg;
      numArgs += 1;
    }
  }

  if (numArgs < m_requireds.size()) {
    cerr << "not enough arguments"
         << " (" << numArgs << " < " << m_requireds.size() << ")\n\n";
    printHelp(argv[0]);
    return true;
  }

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
