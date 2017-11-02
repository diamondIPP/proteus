#include "merger.h"

#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(EventMerger)

Io::EventMerger::EventMerger(
    std::vector<std::shared_ptr<Io::EventReader>> readers)
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
      ERROR("reader ", i, " with inconsistent events reader=", eventsReader,
            " expected=", m_events);
  }
}

std::string Io::EventMerger::name() const { return "EventMerger"; }

void Io::EventMerger::skip(uint64_t n)
{
  for (auto& reader : m_readers)
    reader->skip(n);
}

bool Io::EventMerger::read(Storage::Event& event)
{
  Index isensor = 0;
  for (auto& reader : m_readers) {
    Index nsensors = reader->numSensors();
    // read sub-event
    Storage::Event sub(nsensors);
    if (!reader->read(sub))
      return false;
    // merge into full event
    event.setSensorData(isensor, std::move(sub));
    // where to store the next sensors
    isensor += nsensors;
  }
  // no more events to read if we have no readers to begin with
  return !m_readers.empty();
}
