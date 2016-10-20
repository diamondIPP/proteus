#include "eventprinter.h"

#include <iostream>

#include "storage/event.h"

std::shared_ptr<Analyzers::EventPrinter> Analyzers::EventPrinter::make()
{
  return std::make_shared<EventPrinter>();
}

std::string Analyzers::EventPrinter::name() const { return "EventPrinter"; }

void Analyzers::EventPrinter::analyze(uint64_t eventId,
                                      const Storage::Event& event)
{
  std::cout << "event " << event.id() << ":\n";
  event.print(std::cout, "  ");
}

void Analyzers::EventPrinter::finalize() {}
