#include "eventprinter.h"

#include <iostream>

#include "storage/event.h"

std::string Analyzers::EventPrinter::name() const { return "EventPrinter"; }

void Analyzers::EventPrinter::analyze(const Storage::Event& event)
{
  std::cout << "event:\n";
  event.print(std::cout, "  ");
}

void Analyzers::EventPrinter::finalize() {}
