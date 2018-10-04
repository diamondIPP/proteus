#include "eventprinter.h"

#include <iostream>

#include "storage/event.h"

Analyzers::EventPrinter::EventPrinter() : m_logger("EventPrinter") {}

std::string Analyzers::EventPrinter::name() const { return "EventPrinter"; }

void Analyzers::EventPrinter::execute(const Storage::Event& event)
{
  m_logger.log(Utils::Logger::Level::Info, "event:\n");
  m_logger.logp(Utils::Logger::Level::Info, event, "  ");
}
