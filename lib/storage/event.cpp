#include "event.h"

#include <cassert>
#include <iostream>

Storage::Event::Event(size_t sensors)
    : m_frame(UINT64_MAX)
    , m_timestamp(UINT64_MAX)
    , m_triggerInfo(-1)
    , m_triggerOffset(-1)
    , m_triggerPhase(-1)
{
  m_sensors.reserve(sensors);
  for (size_t isensor = 0; isensor < sensors; ++isensor)
    m_sensors.emplace_back(SensorEvent(isensor));
}

void Storage::Event::clear(uint64_t frame, uint64_t timestamp)
{
  m_frame = frame;
  m_timestamp = timestamp;
  m_triggerInfo = -1;
  m_triggerOffset = -1;
  m_triggerPhase = -1;
  for (auto& sensorEvent : m_sensors)
    sensorEvent.clear(frame, timestamp);
  m_tracks.clear();
}

void Storage::Event::setTrigger(int32_t info, int32_t offset, int32_t phase)
{
  m_triggerInfo = info;
  m_triggerOffset = offset;
  m_triggerPhase = phase;
}

void Storage::Event::setSensorData(Index isensor, SensorEvent&& sensorEvent)
{
  assert(((0 <= isensor) && (isensor < m_sensors.size())) &&
         "Sensor index must be valid");

  m_sensors[isensor] = std::move(sensorEvent);
  m_sensors[isensor].m_sensor = isensor;
  m_sensors[isensor].m_states.clear();
}

void Storage::Event::setSensorData(Index first, Event&& event)
{
  for (Index isensor = 0, n = event.m_sensors.size(); isensor < n; ++isensor)
    setSensorData(first + isensor, std::move(event.m_sensors[isensor]));
}

void Storage::Event::addTrack(std::unique_ptr<Track> track)
{
  m_tracks.emplace_back(std::move(track));
  m_tracks.back()->m_index = m_tracks.size() - 1;
  m_tracks.back()->freezeClusterAssociation();
}

size_t Storage::Event::getNumHits() const
{
  size_t n = 0;
  for (const auto& se : m_sensors)
    n += se.numHits();
  return n;
}

size_t Storage::Event::getNumClusters() const
{
  size_t n = 0;
  for (const auto& se : m_sensors)
    n += se.numClusters();
  return n;
}

void Storage::Event::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "frame: " << frame() << '\n';
  os << prefix << "timestamp: " << timestamp() << '\n';
  for (size_t isensor = 0; isensor < m_sensors.size(); ++isensor) {
    const auto& sensorEvent = m_sensors[isensor];
    // only print non-empty sensor events
    if ((0 < sensorEvent.numHits()) || (0 < sensorEvent.numClusters())) {
      os << prefix << "sensor " << isensor << ":\n";
      sensorEvent.print(os, prefix + "  ");
    }
  }
  if (!m_tracks.empty()) {
    os << prefix << "tracks:\n";
    for (size_t itrack = 0; itrack < m_tracks.size(); ++itrack) {
      os << prefix << "  track " << itrack << ":\n";
      m_tracks[itrack]->print(os, prefix + "    ");
    }
  }
  os.flush();
}
