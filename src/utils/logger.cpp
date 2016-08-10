#include "logger.h"

Utils::Logger& Utils::globalLogger()
{
    static Logger logger;
    return logger;
}
