// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
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
#define ANSI_YELLOW "\x1B[33m"

namespace proteus {

Logger::Logger(Level level)
    : m_level(level)
    , m_streams{&std::cerr, &std::cerr, &std::cout, &std::cout}
    , m_prefixes{ANSI_BOLD ANSI_RED "E", ANSI_BOLD ANSI_YELLOW "W", "I",
                 ANSI_ITALIC "V"}
    , m_reset{ANSI_RESET}
{
}

Logger& globalLogger()
{
  static Logger log;
  return log;
}

} // namespace proteus
