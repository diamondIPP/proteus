// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "eventprinter.h"

#include <iostream>

#include "storage/event.h"

namespace proteus {

EventPrinter::EventPrinter() : m_logger("EventPrinter") {}

std::string EventPrinter::name() const { return "EventPrinter"; }

void EventPrinter::execute(const Event& event)
{
  m_logger.log(Logger::Level::Info, "event:\n");
  m_logger.logp(Logger::Level::Info, event, "  ");
}

} // namespace proteus
