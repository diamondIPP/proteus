// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "logger.h"

#include <ctime>
#include <iomanip>

#define ANSI_RESET "\x1B[0m"
#define ANSI_BOLD "\x1B[1m"
#define ANSI_ITALIC "\x1B[3m"
#define ANSI_RED "\x1B[31m"
#define ANSI_YELLOW "\x1B[33m"

namespace proteus {

// per-level prefix
const char* const Logger::kLevelPrefix[4] = {
    ANSI_BOLD ANSI_RED "E",
    ANSI_BOLD ANSI_YELLOW "W",
    "I",
    ANSI_ITALIC "V",
};
const char* const Logger::kReset = ANSI_RESET;

// global logger w/o specific name
Logger Logger::s_global("");
// default global log-level
Logger::Level Logger::s_level = Logger::Level::Error;

Logger::Logger(std::string name) : m_prefix(std::move(name))
{
  constexpr size_t kPrefixMax = 16;

  m_prefix.resize(kPrefixMax, ' ');
  m_prefix += "| ";
}

Logger& Logger::globalLogger() { return s_global; }

void Logger::print_prefix(std::ostream& os, Level lvl)
{
  std::time_t now = std::time(nullptr);

  os << kLevelPrefix[static_cast<int>(lvl)];
  os << "|";
  os << std::put_time(std::localtime(&now), "%T");
  os << "| ";
}

} // namespace proteus
