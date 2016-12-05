/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10-14
 */

#ifndef PT_ARGUMENTS_H
#define PT_ARGUMENTS_H

#include <cstdint>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Utils {

/** Command line argument parser. */
class Arguments {
public:
  Arguments(std::string description);

  /** Add an optional command line argument. */
  void addOption(char key, std::string name, std::string help);
  /** Add an optional command line argument with a default value. */
  template <typename T>
  void addOption(char key, std::string name, std::string help, T value);
  /** Add a required positional command line argument. */
  void addRequiredArgument(std::string name, std::string help = std::string());

  /** Parse command lines and return true on error. */
  bool parse(int argc, char const* argv[]);

  /** Access an optional argument and convert it to the requested type. */
  template <typename T>
  T option(const std::string& name) const;
  /** Access the i-th argument and convert it to the request type. */
  template <typename T>
  T argument(std::size_t idx) const;

private:
  struct Option {
    const std::string name;
    const std::string help;
    std::string value;
    const char abbreviation;
  };
  struct Position {
    const std::string name;
    const std::string help;
  };

  void printHelp(const std::string& arg0) const;
  Option* find(const std::string& name, char abbreviation);
  const Option* find(const std::string& name, char abbreviation) const;

  const std::string m_description;
  std::vector<Option> m_options;
  std::vector<Position> m_required;
  std::vector<std::string> m_args;
};

/** Default command line arguments for (most) Proteus commands. */
class DefaultArguments : public Arguments {
public:
  DefaultArguments(std::string description);

  std::string input() const { return argument<std::string>(0); }
  std::string outputPrefix() const { return argument<std::string>(1); }
  /** Construct full output path from known output prefix and name. */
  std::string makeOutput(const std::string& name) const;

  std::string device() const;
  std::string geometry() const;
  std::string config() const;
  uint64_t skipEvents() const;
  uint64_t numEvents() const;
};

} // namespace Utils

template <typename T>
inline void Utils::Arguments::addOption(char key,
                                        std::string name,
                                        std::string help,
                                        T value)
{
  // value is always stored as string
  std::ostringstream sval;
  sval << value;
  Option opt = {std::move(name), std::move(help), sval.str(), key};
  m_options.push_back(std::move(opt));
}

template <typename T>
inline T Utils::Arguments::option(const std::string& name) const
{
  const Option* opt = find(name, 0);
  if (!opt) {
    throw std::runtime_error("Arguments: option '" + name +
                             "' does not exists");
  }
  T ret;
  std::istringstream(opt->value) >> ret;
  return ret;
}

template <typename T>
inline T Utils::Arguments::argument(std::size_t idx) const
{
  if (m_args.size() <= idx) {
    throw std::runtime_error("Arguments: argument " + std::to_string(idx) +
                             "does not exists");
  }
  T ret;
  std::istringstream(m_args[idx]) >> ret;
  return ret;
}

#endif // PT_ARGUMENTS_H
