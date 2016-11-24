/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10-14
 */

#ifndef PT_ARGUMENTS_H
#define PT_ARGUMENTS_H

#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Utils {

/** Common command line argument parser for proteus programs. */
class Arguments {
public:
  Arguments(const char* description);

  /** Add an optional commandline argument. */
  template <typename T>
  void addOption(char key, std::string name, std::string help, T value = T());

  /** Parse command lines and return true on error. */
  bool parse(int argc, char const* argv[]);

  const std::string& input() const { return m_input; }
  const std::string& outputPrefix() const { return m_outputPrefix; }
  std::string device() const;
  std::string config() const;
  /** Construct full output path from known output prefix and name. */
  std::string makeOutput(const std::string& name) const;
  /** Access an optional argument and convert it to the requested type. */
  template <typename T>
  T get(const std::string& name) const;

  void print(std::ostream& os) const;

private:
  struct Option {
    const std::string name;
    const std::string help;
    std::string value;
    const char abbreviation;
  };

  Option* find(const std::string& name, char abbreviation);
  const Option* find(const std::string& name, char abbreviation) const;
  void printHelp(const std::string& arg0) const;

  const std::string m_description;
  std::vector<std::string> m_positionals;
  std::vector<Option> m_options;
  std::string m_input, m_outputPrefix;
};

} // namespace Utils

template <typename T>
inline T Utils::Arguments::get(const std::string& name) const
{
  const Option* opt = find(name, 0);
  if (!opt)
    throw std::runtime_error("Arguments: option '" + name +
                             "' does not exists");

  T ret;
  std::istringstream(opt->value) >> ret;
  return ret;
}

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

#endif // PT_ARGUMENTS_H
