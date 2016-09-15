#include "logger.h"

Utils::Logger& Utils::logger()
{
    static Logger s_logger;
    return s_logger;
}
