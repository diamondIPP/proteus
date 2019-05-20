// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#pragma once

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace proteus {
namespace detail {

// enable direct printing of std::vector's
template <typename T>
inline void print_impl(std::ostream& os, const std::vector<T>& things)
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
}
template <typename T>
inline void print_impl(std::ostream& os, const T& thing)
{
  os << thing;
}
// shorter printing w/o << operators
template <typename... Ts>
inline void print(std::ostream& os, const Ts&... things)
{
  using swallow = int[];
  (void)swallow{0, (print_impl(os, things), 0)...};
}

// helper function to suppress unused variables warnings in non-debug builds
template <typename... Ts>
inline void unreferenced(Ts&&...)
{
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
 *     Logger log;
 *     double x = 1.2;
 *     int y = 100;
 *     log.error("x = ", x, " y = ", y, '\n');
 *
 * Each logger has an optional prefix to easily identify the information
 * source. The log level is global and shared among all loggers.
 */
class Logger {
public:
  enum class Level {
    Error = 0,   // fatal error that always stops the program
    Warning = 1, // non-fatal warning that indicate e.g. degraded performance
    Info = 2,    // nice-to-have information
    Verbose = 3, // additional (debug) information
  };

  static void setGlobalLevel(Level lvl) { s_level = lvl; }
  static Logger& globalLogger();

  Logger(std::string name);

  /** Log information with a given log level. */
  template <typename... Ts>
  void log(Level lvl, const Ts&... things) const
  {
    if (isActive(lvl)) {
      std::ostream& os = stream(lvl);
      print_prefix(os, lvl);
      detail::print(os, things...);
      os << kReset;
      os.flush();
    }
  }
  /** Log information using an objects print(...) function. */
  template <typename T>
  void logp(Level lvl,
            const T& thing,
            const std::string& extraPrefix = std::string()) const
  {
    if (isActive(lvl)) {
      std::ostream& os = stream(lvl);
      std::ostringstream prefix;
      print_prefix(prefix, lvl);
      prefix << extraPrefix;
      thing.print(os, prefix.str());
      os << kReset;
      os.flush();
    }
  }

private:
  /** Check whether messages at the give loggging level are active. */
  bool isActive(Level lvl) const { return (lvl <= s_level); }
  static constexpr std::ostream& stream(Level lvl)
  {
    return (lvl == Level::Error) ? std::cerr : std::cout;
  }
  static void print_prefix(std::ostream& os, Level lvl);

  static const char* const kLevelPrefix[4];
  static const char* const kReset;
  static Logger s_global;
  static Level s_level;

  std::string m_prefix;
};

} // namespace proteus

/* Define a `logger()` function that returns either the global logger
 * or a static local logger so the convenience logger macros can be used.
 *
 * Depending on the compile definitions, i.e. when using a debug build with
 * only debug log statements, the functions might be unused and are marked
 * as such to avoid false-positive compiler warnings.
 */

#define PT_SETUP_GLOBAL_LOGGER                                                 \
  namespace {                                                                  \
  inline __attribute__((unused))::proteus::Logger& logger()                    \
  {                                                                            \
    return ::proteus::Logger::globalLogger();                                  \
  }                                                                            \
  }
#define PT_SETUP_LOCAL_LOGGER(name)                                            \
  namespace {                                                                  \
  ::proteus::Logger name##LocalLogger(#name);                                  \
  inline __attribute__((unused))::proteus::Logger& logger()                    \
  {                                                                            \
    return name##LocalLogger;                                                  \
  }                                                                            \
  }

/* Convenience macros to log a message via the logger.
 *
 * The macros should be used to log a single message. The message **must not**
 * end in a newline. These macros expect a `logger()` function to be available
 * that returns a reference to a `Logger` object. This can be either defined
 * at file scope by using the `PT_SETUP_..._LOGGER` macros or by implementing
 * a private class method to use a class-specific logger.
 */
/** `FAIL(...)` should be prefered instead to also terminate the application. */
#define ERROR(...)                                                             \
  do {                                                                         \
    logger().log(Logger::Level::Error, __VA_ARGS__, '\n');                     \
  } while (false)
#define WARN(...)                                                              \
  do {                                                                         \
    logger().log(Logger::Level::Warning, __VA_ARGS__, '\n');                   \
  } while (false)
#define INFO(...)                                                              \
  do {                                                                         \
    logger().log(Logger::Level::Info, __VA_ARGS__, '\n');                      \
  } while (false)
#define VERBOSE(...)                                                           \
  do {                                                                         \
    logger().log(Logger::Level::Verbose, __VA_ARGS__, '\n');                   \
  } while (false)
/* Debug messages are logged with the verbose level but are only shown in
 * a debug build. They become noops in a release build. In a release build
 * the arguments should not be evaluated and no warning for unused variables
 * should be emitted.
 */
#ifdef NDEBUG
#define DEBUG(...)                                                             \
  do {                                                                         \
    (decltype(detail::unreferenced(__VA_ARGS__)))0;                            \
  } while (false)
#else
#define DEBUG(...) VERBOSE(__VA_ARGS__)
#endif

/** Write the error message to the logger and quit the application. */
#define FAIL(...)                                                              \
  do {                                                                         \
    logger().log(Logger::Level::Error, __VA_ARGS__, '\n');                     \
    std::exit(EXIT_FAILURE);                                                   \
  } while (false)
/** Throw an exception of the given type with a custom error message. */
#define THROWX(ExceptionType, ...)                                             \
  do {                                                                         \
    std::ostringstream os;                                                     \
    detail::print(os, __VA_ARGS__);                                            \
    throw ExceptionType(os.str());                                             \
  } while (false)
/** Throw a std::runtime_error with a custom error message. */
#define THROW(...) THROWX(std::runtime_error, __VA_ARGS__)
