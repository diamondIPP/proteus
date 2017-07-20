#include "event.h"

#include <cassert>
#include <iostream>

Storage::Event::Event(size_t sensors)
    : m_frame(UINT64_MAX)
    , m_timestamp(UINT64_MAX)
    , m_triggerInfo(-1)
    , m_triggerOffset(-1)
    , m_triggerPhase(-1)
    , m_invalid(false)
{
  m_planes.reserve(sensors);
  for (size_t isensor = 0; isensor < sensors; ++isensor)
    m_planes.emplace_back(Plane(isensor));
}

void Storage::Event::clear(uint64_t frame, uint64_t timestamp)
{
  m_frame = frame;
  m_timestamp = timestamp;
  m_triggerInfo = -1;
  m_triggerOffset = -1;
  m_triggerPhase = -1;
  m_invalid = false;
  for (auto& plane : m_planes)
    plane.clear();
  m_tracks.clear();
}

void Storage::Event::addTrack(std::unique_ptr<Track> track)
{
  m_tracks.emplace_back(std::move(track));
  m_tracks.back()->m_index = m_tracks.size() - 1;
}

size_t Storage::Event::getNumHits() const
{
  size_t n = 0;
  for (const auto& plane : m_planes)
    n += plane.numHits();
  return n;
}

size_t Storage::Event::getNumClusters() const
{
  size_t n = 0;
  for (const auto& plane : m_planes)
    n += plane.numClusters();
  return n;
}

void Storage::Event::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "frame: " << frame() << '\n';
  os << prefix << "timestamp: " << timestamp() << '\n';
  os << prefix << "invalid: " << invalid() << '\n';
  os << prefix << "sensors:\n";
  for (size_t isensor = 0; isensor < m_planes.size(); ++isensor) {
    const Storage::Plane& sensorEvent = m_planes[isensor];
    // only print non-empty sensor events
    if ((0 < sensorEvent.numHits()) && (0 < sensorEvent.numClusters())) {
      os << prefix << "  sensor " << isensor << ":\n";
      sensorEvent.print(os, prefix + "    ");
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
