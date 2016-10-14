/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10-14
 */

#ifndef PT_ARGUMENTS_H
#define PT_ARGUMENTS_H

#include <map>
#include <ostream>
#include <string>

namespace Utils {

/** Common command line argument parser for proteus programs. */
class Arguments {
public:
  Arguments(const char* description);

  /** Parse command lines and return true on error. */
  bool parse(int argc, char const* argv[]);

  const std::string& input() const { return m_input; }
  const std::string& outputPrefix() const { return m_outputPrefix; }
  const std::string& device() const;
  const std::string& config() const;
  /** Construct full output path from known output prefix and name. */
  std::string makeOutput(const std::string& name) const;

  void print(std::ostream& os) const;

private:
  void printHelp(const std::string& arg0);

  const std::string m_description;
  std::string m_input, m_outputPrefix;
  std::map<std::string, std::string> m_opts;
};

} // namespace Utils

#endif // PT_ARGUMENTS_H
