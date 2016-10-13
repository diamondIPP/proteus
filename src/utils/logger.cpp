#include "logger.h"

const char* Utils::Logger::ANSI_RESET = "\x1B[0m";
const char* Utils::Logger::ANSI_BOLD = "\x1B[1m";
const char* Utils::Logger::ANSI_ITALIC = "\x1B[3m";
const char* Utils::Logger::ANSI_RED = "\x1B[31m";

Utils::Logger& Utils::logger()
{
  static Logger s_logger;
  return s_logger;
}
