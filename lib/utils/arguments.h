/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10-14
 */

#pragma once

#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace proteus {

/** Command line argument parser. */
class Arguments {
public:
  Arguments(std::string description);

  /** Add a command line option without parameter. */
  void addFlag(char key, std::string name, std::string help);
  /** Add a command line option. */
  void addOption(char key, std::string name, std::string help);
  /** Add a command line option with a default value. */
  template <typename T>
  void addOption(char key, std::string name, std::string help, T value);
  /** Add a command line option that can be given multiple times. */
  void addOptionMulti(char key, std::string name, std::string help);
  /** Add a required command line argument. */
  void addRequired(std::string name, std::string help = std::string());
  /** Add an optional command line argument with a default value.
   *
   * These are always parsed after all require arguments regardless of their
   * definition order.
   */
  template <typename T>
  void addOptional(std::string name, std::string help, T value);
  /** Allow additional command line arguments at the end.
   *
   * This will contain left-over arguments after the required and optional
   * arguments have been passed. This should be called at most once.
   */
  void addVariable(std::string name, std::string help = std::string());

  /** Parse command lines and return true on error. */
  bool parse(int argc, char const* argv[]);

  /** Check if a given argument exists. */
  bool has(const std::string& name) const;
  /** Return argument value. */
  const std::string& get(const std::string& name) const;
  /** Return argument value with automatic conversion to selected type. */
  template <typename T>
  T get(const std::string& name) const;

private:
  static constexpr char kNoAbbr = '\0';

  enum class OptionType { kFlag, kSingle, kMulti };
  struct Option {
    std::string name;
    std::string help;
    std::string defaultValue = std::string();
    char abbreviation = kNoAbbr;
    OptionType type = OptionType::kSingle;

    std::string description() const;
  };
  struct RequiredArgument {
    std::string name;
    std::string help;
  };
  struct OptionalArgument {
    std::string name;
    std::string help;
    std::string defaultValue;
  };

  template <typename T>
  static void fromString(const std::string& in, std::vector<T>& out);
  template <typename T>
  static void fromString(const std::string& in, T& out);

  void printHelp(const std::string& arg0) const;
  const Option* find(const std::string& name) const;
  const Option* find(char abbreviation) const;

  const std::string m_description;
  std::vector<Option> m_options;
  std::vector<RequiredArgument> m_requireds;
  std::vector<OptionalArgument> m_optionals;
  RequiredArgument m_variable;
  std::map<std::string, std::string> m_values;
};

// inline implementations

template <typename T>
inline void
Arguments::addOption(char key, std::string name, std::string help, T value)
{
  // value is always stored as string
  std::ostringstream sval;
  sval << value;
  Option opt;
  opt.abbreviation = key;
  opt.name = std::move(name);
  opt.help = std::move(help);
  opt.defaultValue = sval.str();
  m_options.push_back(std::move(opt));
}

template <typename T>
inline void Arguments::addOptional(std::string name, std::string help, T value)
{
  // value is always stored as string
  std::ostringstream sval;
  sval << value;
  OptionalArgument opt;
  opt.name = std::move(name);
  opt.help = std::move(help);
  opt.defaultValue = sval.str();
  m_optionals.push_back(std::move(opt));
}

inline bool Arguments::has(const std::string& name) const
{
  return (m_values.count(name) == 1);
}

inline const std::string& Arguments::get(const std::string& name) const
{
  return m_values.at(name);
}

template <typename T>
inline void Arguments::fromString(const std::string& in, T& out)
{
  std::istringstream(in) >> out;
}

template <typename T>
inline void Arguments::fromString(const std::string& in, std::vector<T>& out)
{
  // multiple values are stored in a single string separated by kommas,
  size_t start = 0;
  size_t end = std::string::npos;
  while (start != std::string::npos) {
    end = in.find_first_of(',', start);

    T element;
    if (end != std::string::npos) {
      // parse substring until the separator
      fromString(in.substr(start, end - start), element);
      start = end + 1;
    } else {
      // no separator was found; use the remaining string
      fromString(in.substr(start), element);
      start = std::string::npos;
    }
    out.emplace_back(std::move(element));
  }
}

template <typename T>
inline T Arguments::get(const std::string& name) const
{
  if (!has(name))
    throw std::runtime_error("Arguments: unknown argument '" + name + "'");
  T ret;
  fromString(m_values.at(name), ret);
  return ret;
}
} // namespace proteus
