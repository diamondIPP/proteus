#include "io.h"

#include "io/rceroot.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Io);

Io::EventReader::~EventReader() {}

Io::EventWriter::~EventWriter() {}

std::shared_ptr<Io::EventReader> Io::openRead(const std::string& path)
{
  if (Io::RceRootReader::isValid(path))
    return std::make_shared<RceRootReader>(path);
  // TODO add more reader options once they become available
  FAIL("'", path, "' is unreadable or not a valid input format");
  return nullptr; // will not be reached
}
