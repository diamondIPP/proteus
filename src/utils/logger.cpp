/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "logger.h"

#define ANSI_RESET "\x1B[0m"
#define ANSI_BOLD "\x1B[1m"
#define ANSI_ITALIC "\x1B[3m"
#define ANSI_RED "\x1B[31m"

// per-level prefix
// clang-format off
const char* const Utils::Logger::kLevelPrefix[3] = {
    ANSI_BOLD ANSI_RED "ERROR|",
                       "INFO |",
    ANSI_ITALIC        "DEBUG|",
};
const char* const Utils::Logger::kReset = ANSI_RESET;
// clang-format on

// global logger w/o specific name
Utils::Logger Utils::Logger::s_global("");
// default global log-level
Utils::Logger::Level Utils::Logger::s_level = Utils::Logger::Level::Info;

Utils::Logger::Logger(std::string name) : m_prefix(std::move(name))
{
  if (!m_prefix.empty()) {
    m_prefix += "| ";
  } else {
    m_prefix = ' ';
  }
}

Utils::Logger& Utils::Logger::globalLogger() { return s_global; }
