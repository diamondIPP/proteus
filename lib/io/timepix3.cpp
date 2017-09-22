#include "timepix3.h"

#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Timepix3)

Io::Timepix3Reader::Timepix3Reader(const std::string& path) {
  INFO("Timepix3REader::constructor");
}

Io::Timepix3Reader::~Timepix3Reader()
{
  INFO("Timepix3Reader::deconstructor");
}

std::string Io::Timepix3Reader::name() const { return "Timepix3Reader"; }

void Io::Timepix3Reader::skip(uint64_t n)
{
}

bool Io::Timepix3Reader::read(Storage::Event& event)
{  
  INFO("Timepix3Reader::read");
  return true;
}
