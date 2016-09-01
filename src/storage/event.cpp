#include "event.h"

#include <cassert>
#include <iostream>

#include "cluster.h"
#include "hit.h"
#include "plane.h"
#include "track.h"

using Storage::Hit;
using Storage::Cluster;
using Storage::Track;
using Storage::Plane;

//=========================================================
Storage::Event::Event(unsigned int numPlanes)
    : _timeStamp(0)
    , _frameNumber(0)
    , _triggerOffset(0)
    , _invalid(false)
{
  for (unsigned int nplane = 0; nplane < getNumPlanes(); nplane++) {
    _planes.push_back(Plane(nplane));
  }
}

//=========================================================
Storage::Hit* Storage::Event::newHit(unsigned int nplane)
{
  _hits.push_back(Hit());
  _planes.at(nplane).addHit(&_hits.back());
  return &_hits.back();
}

//=========================================================
Cluster* Storage::Event::newCluster(unsigned int nplane)
{
  _clusters.push_back(Cluster());
  _clusters.back().m_index = _clusters.size() - 1;
  _planes.at(nplane).addCluster(&_clusters.back());
  return &_clusters.back();
}

//=========================================================
void Storage::Event::addTrack(Track* otherTrack)
{
  // WARNING 2016-08-25 msmk: implicit ownership transfer
  _tracks.push_back(*otherTrack);
  _tracks.back().m_index = _tracks.size() - 1;
  delete otherTrack;
}

//=========================================================
Track* Storage::Event::newTrack()
{
  _tracks.push_back(Track());
  _tracks.back().m_index = _tracks.size() - 1;
  return &_tracks.back();
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
  for (const auto& plane : _planes) {
    os << prefix << "  plane " << iplane++ << ":\n";
    plane.print(os, prefix + "    ");
  }

  os << prefix << "tracks:\n";
  for (const auto& track : _tracks) {
    os << prefix << "  track " << itrack++ << ":\n";
    track.print(os, prefix + "    ");
  }

  os.flush();
}

void Storage::Event::print() const { print(std::cout); }
