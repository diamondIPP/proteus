#include "event.h"

#include <cassert>
#include <iostream>

Storage::Event::Event(Index numPlanes)
    : m_timeStamp(0)
    , m_frameNumber(0)
    , m_triggerOffset(0)
    , m_invalid(false)
{
  for (Index iplane = 0; iplane < numPlanes; ++iplane) {
    m_planes.push_back(Plane(iplane));
  }
}

Storage::Hit* Storage::Event::newHit(Index iplane)
{
  m_hits.push_back(Hit());
  m_planes.at(iplane).addHit(&m_hits.back());
  return &m_hits.back();
}

Storage::Cluster* Storage::Event::newCluster(Index iplane)
{
  m_clusters.push_back(Cluster());
  m_clusters.back().m_index = m_clusters.size() - 1;
  m_planes.at(iplane).addCluster(&m_clusters.back());
  return &m_clusters.back();
}

void Storage::Event::addTrack(Track* otherTrack)
{
  // WARNING 2016-08-25 msmk: implicit ownership transfer
  m_tracks.push_back(*otherTrack);
  m_tracks.back().m_index = m_tracks.size() - 1;
  delete otherTrack;
}

Storage::Track* Storage::Event::newTrack()
{
  m_tracks.push_back(Track());
  m_tracks.back().m_index = m_tracks.size() - 1;
  return &m_tracks.back();
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
    track.print(os, prefix + "    ");
  }

  os.flush();
}

void Storage::Event::print() const { print(std::cout); }
