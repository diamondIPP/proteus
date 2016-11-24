/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_LOGGER_H
#define PT_LOGGER_H

#include <iostream>
#include <string>
#include <vector>

namespace Utils {

/** A logger object with global log level.
 *
 * The logger provides logging functions for each predefined log level. Each
 * function, `error`, `info`, debug` can be called with a variable number of
 * arguments. They will be printed in the given order to a predefined stream.
 * Any type for which an `operator<<(std::ostream&, ...)` is defined can be
 * used, e.g.
 *
 *     Utils::Logger log;
 *     double x = 1.2;
 *     int y = 100;
 *     log.error("x = ", x, " y = ", y, '\n');
 *
 * Each logger has an optional prefix to easily identify the information
 * source. The log level is global and shared among all loggers.
 */
class Logger {
public:
  enum class Level { Error = 0, Info = 1, Debug = 2 };

  static void setGlobalLevel(Level lvl) { s_level = lvl; }
  static Logger& globalLogger();

  Logger(std::string name);

  template <typename... Ts>
  void error(const Ts&... things)
  {
    log(Level::Error, things...);
  }
  template <typename... Ts>
  void info(const Ts&... things)
  {
    log(Level::Info, things...);
  }
  template <typename... Ts>
  void debug(const Ts&... things)
  {
    log(Level::Debug, things...);
  }

private:
  template <typename T>
  static void print(std::ostream& os, const T& thing)
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
    return (lvl <= Level::Error) ? std::cerr : std::cout;
  }
  template <typename... Ts>
  void log(Level lvl, const Ts&... things)
  {
    if (lvl <= s_level)
      print(stream(lvl), kLevelPrefix[static_cast<int>(lvl)], m_prefix,
            things..., kReset);
  }

  static const char* const kLevelPrefix[3];
  static const char* const kReset;
  static Logger s_global;
  static Level s_level;
  std::string m_prefix;
};

// print utilities for std containers

template <typename Container>
std::ostream& printContainer(std::ostream& os, const Container& things)
{
  auto it = std::begin(things);
  auto end = std::end(things);
  os << '[';
  if (it != end)
    os << *it;
  for (++it; it != end; ++it)
    os << ", " << *it;
  os << ']';
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& things)
{
  return printContainer(os, things);
}

} // namespace Utils

/* Define a `logger()` function that returns either the global logger
 * or a static local logger.
 */
#define PT_SETUP_GLOBAL_LOGGER                                                 \
  static inline Utils::Logger& logger()                                        \
  {                                                                            \
    return Utils::Logger::globalLogger();                                      \
  }
#define PT_SETUP_LOCAL_LOGGER(name)                                            \
  static Utils::Logger name##LocalLogger(#name);                               \
  static inline Utils::Logger& logger() { return name##LocalLogger; }

/* Convenience macros to log a message use the logger.
 *
 * The macros should be used to log a single message. The message **must not**
 * end in a newline.
 *
 * These macros expect a `logger()` function to be available that
 * returns a reference to a `Logger` object. This can be either defined
 * at file scope by using the `PT_SETUP_..._LOGGER` macros or by implementing
 * a private class method to use a class-specific logger.
 */
#define ERROR(...)                                                             \
  do {                                                                         \
    logger().error(__VA_ARGS__, '\n');                                         \
  } while (false)
#define INFO(...)                                                              \
  do {                                                                         \
    logger().info(__VA_ARGS__, '\n');                                          \
  } while (false)
#define DEBUG(...)                                                             \
  do {                                                                         \
    logger().debug('(', __FILE__, ':', __LINE__, ") ", __VA_ARGS__, '\n');     \
  } while (false)

#endif // PT_LOGGER_H
