/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_LOGGER_H
#define PT_LOGGER_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Utils {
namespace detail {

// support << operator for std::vector
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& things)
{
  bool first = true;
  os << '[';
  for (const auto& t : things) {
    if (first) {
      first = false;
    } else {
      os << ", ";
    }
    os << t;
  }
  os << ']';
  return os;
}

// simple printing w/o << operators
template <typename T>
void print(std::ostream& os, T&& thing)
{
  os << thing;
}
template <typename T0, typename... TN>
void print(std::ostream& os, T0&& thing, TN&&... rest)
{
  os << thing;
  print(os, rest...);
}

} // namespace detail

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
  static bool isActive(Level lvl) { return (lvl <= s_level); }
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
  /** Log debug information using an objects print(...) function. */
  template <typename T>
  void debugp(const T& thing, const std::string& extraPrefix = std::string())
  {
    if (isActive(Level::Debug)) {
      thing.print(stream(Level::Debug), prefix(Level::Debug) + extraPrefix);
      stream(Level::Debug) << kReset;
    }
  }

private:
  static std::ostream& stream(Level lvl)
  {
    return (lvl == Level::Error) ? std::cerr : std::cout;
  }
  std::string prefix(Level lvl) const
  {
    return kLevelPrefix[static_cast<int>(lvl)] + m_prefix;
  }
  template <typename... Ts>
  void log(Level lvl, const Ts&... things)
  {
    if (isActive(lvl))
      detail::print(stream(lvl), prefix(lvl), things..., kReset);
  }

  static const char* const kLevelPrefix[3];
  static const char* const kReset;
  static Logger s_global;
  static Level s_level;

  std::string m_prefix;
};

} // namespace Utils

/* Define a `logger()` function that returns either the global logger
 * or a static local logger so the convenience logger macros can be used.
 *
 * Depending on the compile definitions, i.e. when using a debug build with
 * only debug log statements, the functions might be unused and are marked
 * as such to avoid false-positive compiler warnings.
 */
#define PT_SETUP_GLOBAL_LOGGER                                                 \
  namespace {                                                                  \
  inline __attribute__((unused)) Utils::Logger& logger()                       \
  {                                                                            \
    return Utils::Logger::globalLogger();                                      \
  }                                                                            \
  }
#define PT_SETUP_LOCAL_LOGGER(name)                                            \
  namespace {                                                                  \
  Utils::Logger name##LocalLogger(#name);                                      \
  inline __attribute__((unused)) Utils::Logger& logger()                       \
  {                                                                            \
    return name##LocalLogger;                                                  \
  }                                                                            \
  }

/* Convenience macros to log a message via the logger.
 *
 * The macros should be used to log a single message. The message **must not**
 * end in a newline. The `DEBUG(...)` macros is a noop in non-debug builds.
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
#ifdef NDEBUG
#define DEBUG(...) ((void)0)
#else
#define DEBUG(...)                                                             \
  do {                                                                         \
    logger().debug(__VA_ARGS__, " (", __FUNCTION__, ':', __LINE__, ")\n");     \
  } while (false)
#endif

/** Write the error message w/ the logger and quit the application. */
#define FAIL(...)                                                              \
  do {                                                                         \
    logger().error(__VA_ARGS__, '\n');                                         \
    std::exit(EXIT_FAILURE);                                                   \
  } while (false)

/** Throw an exception of the given type with a custom error message. */
#define THROWX(ExceptionType, ...)                                             \
  do {                                                                         \
    std::ostringstream os;                                                     \
    Utils::detail::print(os, __VA_ARGS__);                                     \
    throw ExceptionType(os.str());                                             \
  } while (false)
/** Throw a std::runtimer_error with a custom error message. */
#define THROW(...) THROWX(std::runtime_error, __VA_ARGS__)

#endif // PT_LOGGER_H
