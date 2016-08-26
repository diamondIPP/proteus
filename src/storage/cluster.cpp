#include "cluster.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

#include "hit.h"
#include "plane.h"
#include "track.h"

using std::cout;
using std::endl;

using Storage::Cluster;
using Storage::Hit;
using Storage::Track;

Storage::Cluster::Cluster()
    : _plane(0)
    , _index(-1)
    , _posX(0)
    , _posY(0)
    , _posZ(0)
    , _posErrX(0)
    , _posErrY(0)
    , _posErrZ(0)
    , _timing(0)
    , _value(0)
    , _matchDistance(0)
    , _track(0)
    , _matchedTrack(0)
    , _numHits(0)
{
}

void Storage::Cluster::setTrack(Storage::Track* track)
{
  assert(!_track && "Cluster: can't use a cluster for more than one track");
  _track = track;
}

void Storage::Cluster::setMatchedTrack(Storage::Track* track)
{
  _matchedTrack = track;
}

void Storage::Cluster::addHit(Storage::Hit* hit)
{
  hit->setCluster(this);
  _hits.push_back(hit);
  // Fill the value and timing from this hit
  _value += hit->getValue();
  if (_numHits == 0)
    _timing = hit->getTiming();
  _numHits++;
}

Hit* Storage::Cluster::getHit(unsigned int n) const
{
  assert(n < getNumHits() && "Cluster: hit index exceeds vector range");
  return _hits.at(n);
}

Track* Storage::Cluster::getTrack() const { return _track; }

Track* Storage::Cluster::getMatchedTrack() const { return _matchedTrack; }

Storage::Plane* Storage::Cluster::getPlane() const { return _plane; }

void Storage::Cluster::print() const
{
  cout << "\nCLUSTER\n" << printStr() << endl;
}

const std::string Storage::Cluster::printStr(int blankWidth) const
{
  std::ostringstream out;

  std::string blank(" ");
  for (int i = 1; i <= blankWidth; i++)
    blank += " ";

  out << blank << "Address: " << this << "\n";
  out << blank << "Pix: (" << getPixX() << " , " << getPixY() << ")\n";
  out << blank << "Pix err: (" << getPixErrX() << " , " << getPixErrY()
      << ")\n";
  out << blank << "Pos: (" << getPosX() << " , " << getPosY() << " , "
      << getPosZ() << ")\n";
  out << blank << "Pos err: (" << getPosErrX() << " , " << getPosErrY() << " , "
      << getPosErrZ() << ")\n";
  out << blank << "Num hits: " << getNumHits() << "\n";
  out << blank << "Plane: " << getPlane();

  return out.str();
}
