#include "event.h"

#include <cassert>
#include <iostream>
#include <vector>

#include <Rtypes.h>

#include "cluster.h"
#include "hit.h"
#include "plane.h"
#include "track.h"

using std::cout;
using std::endl;

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
void Storage::Event::print()
{
  cout << "\nEVENT:\n"
       << "  Time stamp: " << getTimeStamp() << "\n"
       << "  Frame number: " << getFrameNumber() << "\n"
       << "  Trigger offset: " << getTriggerOffset() << "\n"
       << "  Invalid: " << getInvalid() << "\n"
       << "  Num planes: " << getNumPlanes() << "\n"
       << "  Num hits: " << getNumHits() << "\n"
       << "  Num clusters: " << getNumClusters() << endl;

  for (unsigned int nplane = 0; nplane < getNumPlanes(); nplane++) {
    cout << "\n[" << nplane << "] ";
    getPlane(nplane)->print();
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
  _tracks.back()._index = _tracks.size() - 1;
  delete otherTrack;
}

//=========================================================
Track* Storage::Event::newTrack()
{
  _tracks.push_back(Track());
  _tracks.back()._index = _tracks.size() - 1;
  return &_tracks.back();
}
