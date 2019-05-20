// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#pragma once

#include <cstdlib>
#include <ctime>
#include <iomanip>
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

/** A logger object with adjustable log level.
 *
 * The logger provides a `.log(...)` function that can be called with a
 * variable number of arguments. They will be printed in the given order to
 * a level-dependent stream. Any type for which an
 * `operator<<(std::ostream&, ...)` is defined can be used, e.g.
 *
 *     Logger log;
 *     double x = 1.2;
 *     int y = 100;
 *     log.log(Logger::Level::Warning, "x = ", x, " y = ", y, '\n');
 *
 */
class Logger {
public:
  enum class Level {
    Error = 0,   // fatal error that always stops the program
    Warning = 1, // non-fatal warning that indicate e.g. degraded performance
    Info = 2,    // nice-to-have information
    Verbose = 3, // additional (debug) information
  };

  Logger(Level level = Level::Warning);

  /** Set the minimal logging level for which messages are shown. */
  void setMinimalLevel(Level level) { m_level = level; }

  /** Log information with a given log level. */
  template <typename... Ts>
  void log(Level lvl, const Ts&... things) const
  {
    if (isActive(lvl)) {
      std::ostream& os = stream(lvl);
      print_prefix(os, lvl);
      detail::print(os, things..., m_reset);
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
      os << m_reset;
      os.flush();
    }
  }

private:
  /** Check whether messages at the give loggging level are active. */
  bool isActive(Level level) const { return (level <= m_level); }
  std::ostream& stream(Level level) const
  {
    return *(m_streams[static_cast<int>(level)]);
  }
  void print_prefix(std::ostream& os, Level level) const
  {
    std::time_t now = std::time(nullptr);
    os << m_prefixes[static_cast<int>(level)];
    os << "|";
    os << std::put_time(std::localtime(&now), "%T");
    os << "| ";
  }

  Level m_level;
  std::ostream* const m_streams[4];
  const char* const m_prefixes[4];
  const char* const m_reset;
};

/** Return the global logger. */
Logger& globalLogger();

} // namespace proteus

/* Convenience macros to log a message via the logger.
 *
 * The macros should be used to log a single message. The message **must not**
 * end in a newline. These macros expect a `globalLogger()` function to be
 * available that returns a reference to a `Logger` object.
 */
/** `FAIL(...)` should be prefered which also terminates the application. */
#define ERROR(...)                                                             \
  do {                                                                         \
    ::proteus::globalLogger().log(::proteus::Logger::Level::Error,             \
                                  __VA_ARGS__, '\n');                          \
  } while (false)
#define WARN(...)                                                              \
  do {                                                                         \
    ::proteus::globalLogger().log(::proteus::Logger::Level::Warning,           \
                                  __VA_ARGS__, '\n');                          \
  } while (false)
#define INFO(...)                                                              \
  do {                                                                         \
    ::proteus::globalLogger().log(::proteus::Logger::Level::Info, __VA_ARGS__, \
                                  '\n');                                       \
  } while (false)
#define VERBOSE(...)                                                           \
  do {                                                                         \
    ::proteus::globalLogger().log(::proteus::Logger::Level::Verbose,           \
                                  __VA_ARGS__, '\n');                          \
  } while (false)
/* Debug messages are logged with the verbose level but are only shown in
 * a debug build. They become noops in a release build. In a release build
 * the arguments should not be evaluated and no warning for unused variables
 * should be emitted.
 */
#ifdef NDEBUG
#define DEBUG(...)                                                             \
  do {                                                                         \
    (decltype(::proteus::detail::unreferenced(__VA_ARGS__)))0;                 \
  } while (false)
#else
#define DEBUG(...) VERBOSE(__VA_ARGS__)
#endif

/** Write the error message to the logger and quit the application. */
#define FAIL(...)                                                              \
  do {                                                                         \
    ::proteus::globalLogger().log(::proteus::Logger::Level::Error,             \
                                  __VA_ARGS__, '\n');                          \
    std::exit(EXIT_FAILURE);                                                   \
  } while (false)
/** Throw an exception of the given type with a custom error message. */
#define THROWX(ExceptionType, ...)                                             \
  do {                                                                         \
    std::ostringstream os;                                                     \
    ::proteus::detail::print(os, __VA_ARGS__);                                 \
    throw ExceptionType(os.str());                                             \
  } while (false)
/** Throw a std::runtime_error with a custom error message. */
#define THROW(...) THROWX(std::runtime_error, __VA_ARGS__)
