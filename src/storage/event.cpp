#include "event.h"

#include <cassert>
#include <iostream>

Storage::Event::Event(Index numPlanes)
    : m_id(-1)
    , m_timeStamp(0)
    , m_frameNumber(0)
    , m_triggerOffset(0)
    , m_invalid(false)
{
  for (Index iplane = 0; iplane < numPlanes; ++iplane) {
    m_planes.push_back(Plane(iplane));
  }
}

void Storage::Event::clear()
{
  for (auto plane = m_planes.begin(); plane != m_planes.end(); ++plane) {
    plane->clear();
  }
  m_tracks.clear();
}

Storage::Track* Storage::Event::addTrack(const Track& otherTrack)
{
  m_tracks.emplace_back(new Track(otherTrack));
  m_tracks.back()->m_index = m_tracks.size() - 1;
  return m_tracks.back().get();
}

Storage::Track* Storage::Event::newTrack()
{
  m_tracks.emplace_back(new Track());
  m_tracks.back()->m_index = m_tracks.size() - 1;
  return m_tracks.back().get();
}

unsigned int Storage::Event::getNumHits() const
{
  Index n = 0;
  for (auto plane = m_planes.begin(); plane != m_planes.end(); ++plane)
    n += plane->numHits();
  return n;
}
unsigned int Storage::Event::getNumClusters() const
{
  Index n = 0;
  for (auto plane = m_planes.begin(); plane != m_planes.end(); ++plane)
    n += plane->numClusters();
  return n;
}

void Storage::Event::print(std::ostream& os, const std::string& prefix) const
{
  size_t iplane = 0;
  size_t itrack = 0;

  os << prefix << "time stamp: " << getTimeStamp() << '\n';
  os << prefix << "frame number: " << getFrameNumber() << '\n';
  os << prefix << "trigger offset: " << getTriggerOffset() << '\n';
  os << prefix << "invalid: " << getInvalid() << '\n';

  os << prefix << "planes:\n";
  for (const auto& plane : m_planes) {
    os << prefix << "  plane " << iplane++ << ":\n";
    plane.print(os, prefix + "    ");
  }

  os << prefix << "tracks:\n";
  for (const auto& track : m_tracks) {
    os << prefix << "  track " << itrack++ << ":\n";
    track->print(os, prefix + "    ");
  }

  os.flush();
}

void Storage::Event::print() const { print(std::cout); }
