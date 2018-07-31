#include "eventprinter.h"

#include <iostream>

#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(EventPrinter)

std::string Analyzers::EventPrinter::name() const { return "EventPrinter"; }

void Analyzers::EventPrinter::analyze(const Storage::Event& event)
{
  logger().debug("event:\n");
  logger().debugp(event, "  ");
}
