/**
 * @brief Logging utilities
 * @author Moritz Kiehn <msmk@cern.ch>
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <iostream>

namespace Utils {

/** A logger object with predefined log level.
 *
 * The logger provides a set of logging functions, one for each
 * predefined log level. Each function, `error`, `info`, debug` can be
 * called with a variable number of arguments that will be printed in
 * the given order to a predefined stream. Any type for which an
 * `operator<<(std::ostream&, ...)` is defined can be used, e.g.
 *
 *     Utils::Logger log;
 *     double x = 1.2;
 *     int y = 100;
 *     log.error("x = ", x, " y = ", y, '\n');
 *
 */
class Logger {
public:
  enum Level { ERROR = 0, INFO, DEBUG };

  explicit Logger(Level lvl = Level::ERROR)
      : _lvl(lvl)
  {
  }
  void setLevel(Level lvl) { _lvl = lvl; }
  template <typename... Ts> void error(const Ts&... things)
  {
    log(Level::ERROR, things...);
  }
  template <typename... Ts> void info(const Ts&... things)
  {
    log(Level::INFO, things...);
  }
  template <typename... Ts> void debug(const Ts&... things)
  {
    log(Level::DEBUG, things...);
  }

private:
  template <typename T> static void print(std::ostream& os, const T& thing)
  {
    os << thing;
  }
  template <typename T0, typename... TN>
  static void print(std::ostream& os, const T0& thing, const TN&... rest)
  {
    os << thing;
    print(os, rest...);
  }
  static std::ostream& stream(Level lvl)
  {
    return (lvl <= Level::ERROR) ? std::cerr : std::cout;
  }
  template <typename... Ts> void log(Level lvl, const Ts&... things)
  {
    if (lvl <= _lvl)
      print(stream(lvl), things...);
  }

  Level _lvl;
};

Logger& logger();

}  // namespace Utils

/* Convenience macros to use the logger.
 *
 * These macros expect a `logger()` function to be available that
 * returns a reference to a `Logger` object. This allows usage of a
 * class specific logger by implementing the necessary private method
 * of the given name or to use the global logger by importing the
 * global logger function at the beginning of the file, i.e.
 *
 *     using Utils::logger;
 *
 */
#define ERROR(...) do { logger().error(__VA_ARGS__); } while(false)
#define INFO(...) do { logger().info(__VA_ARGS__); } while(false)
#define DEBUG(...) do { logger().debug(__VA_ARGS__); } while(false)

#endif  // __LOGGER_H__
