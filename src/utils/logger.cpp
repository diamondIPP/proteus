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

const static size_t kPrefixMax = 16;

// per-level prefix
// clang-format off
const char* const Utils::Logger::kLevelPrefix[3] = {
    ANSI_BOLD ANSI_RED "E|",
                       "I|",
    ANSI_ITALIC        "D|",
};
// clang-format on
const char* const Utils::Logger::kReset = ANSI_RESET;

// global logger w/o specific name
Utils::Logger Utils::Logger::s_global("");
// default global log-level
Utils::Logger::Level Utils::Logger::s_level = Utils::Logger::Level::Info;

Utils::Logger::Logger(std::string name) : m_prefix(std::move(name))
{
  m_prefix.resize(kPrefixMax, ' ');
  m_prefix += "| ";
}

Utils::Logger& Utils::Logger::globalLogger() { return s_global; }
