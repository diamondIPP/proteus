// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "merger.h"

#include "storage/event.h"
#include "utils/logger.h"

namespace proteus {

EventMerger::EventMerger(std::vector<std::shared_ptr<Reader>> readers)
    : m_readers(std::move(readers)), m_events(UINT64_MAX), m_sensors(0)
{
  // define available events and count sensors
  for (const auto& reader : m_readers) {
    m_events = std::min(m_events, reader->numEvents());
    m_sensors += reader->numSensors();
  }
  // check for consistency
  for (size_t i = 0; i < m_readers.size(); ++i) {
    uint64_t eventsReader = m_readers[i]->numEvents();
    if (eventsReader != m_events)
      WARN("reader ", i, " with inconsistent events reader=", eventsReader,
           " expected=", m_events);
  }
}

std::string EventMerger::name() const { return "EventMerger"; }

void EventMerger::skip(uint64_t n)
{
  for (auto& reader : m_readers)
    reader->skip(n);
}

bool EventMerger::read(Event& event)
{
  size_t ireader = 0;
  Index isensor = 0;

  for (auto& reader : m_readers) {
    Index nsensors = reader->numSensors();
    // read sub-event
    Event sub(nsensors);
    if (!reader->read(sub))
      return false;
    // use first reader to define event number and timestamp
    if (ireader == 0) {
      event.clear(sub.frame(), sub.timestamp());
    }
    // merge into full event
    event.setSensorData(isensor, std::move(sub));
    // where to store the next sensors
    isensor += nsensors;
    ireader += 1;
  }
  // no more events to read if we have no readers to begin with
  return !m_readers.empty();
}

} // namespace proteus
