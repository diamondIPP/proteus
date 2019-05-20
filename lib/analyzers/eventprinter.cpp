// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "eventprinter.h"

#include <iostream>

#include "storage/event.h"
#include "utils/logger.h"

namespace proteus {

EventPrinter::EventPrinter() {}

std::string EventPrinter::name() const { return "EventPrinter"; }

void EventPrinter::execute(const Event& event)
{
  globalLogger().log(Logger::Level::Info, "event:\n");
  globalLogger().logp(Logger::Level::Info, event, "  ");
}

} // namespace proteus
