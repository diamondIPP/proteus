/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10-14
 */

#ifndef PT_ARGUMENTS_H
#define PT_ARGUMENTS_H

#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Utils {

/** Command line argument parser. */
class Arguments {
public:
  Arguments(std::string description);

  /** Add an optional boolean flag. */
  void addFlag(char key, std::string name, std::string help);
  /** Add an optional command line flag. */
  void addOptional(char key, std::string name, std::string help);
  /** Add an optional command line flag with a default value. */
  template <typename T>
  void addOptional(char key, std::string name, std::string help, T value);
  /** Add an optional command line flag that can be given multiple times. */
  void addMulti(char key, std::string name, std::string help);
  /** Add a required command line argument. */
  void addRequired(std::string name, std::string help = std::string());

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
  struct Required {
    std::string name;
    std::string help;
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
  std::vector<Required> m_requireds;
  std::map<std::string, std::string> m_values;
};

} // namespace Utils

template <typename T>
inline void Utils::Arguments::addOptional(char key,
                                          std::string name,
                                          std::string help,
                                          T value)
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

inline bool Utils::Arguments::has(const std::string& name) const
{
  return (m_values.count(name) == 1);
}

inline const std::string& Utils::Arguments::get(const std::string& name) const
{
  return m_values.at(name);
}

template <typename T>
inline void Utils::Arguments::fromString(const std::string& in, T& out)
{
  std::istringstream(in) >> out;
}

template <typename T>
void Utils::Arguments::fromString(const std::string& in, std::vector<T>& out)
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
inline T Utils::Arguments::get(const std::string& name) const
{
  if (!has(name))
    throw std::runtime_error("Arguments: unknown argument '" + name + "'");
  T ret;
  fromString(m_values.at(name), ret);
  return ret;
}

#endif // PT_ARGUMENTS_H
