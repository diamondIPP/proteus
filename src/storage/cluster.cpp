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
    : m_plane(0)
    , m_index(-1)
    , m_posX(0)
    , m_posY(0)
    , m_posZ(0)
    , m_posErrX(0)
    , m_posErrY(0)
    , m_posErrZ(0)
    , m_timing(0)
    , m_value(0)
    , m_matchDistance(0)
    , m_track(0)
    , m_matchedTrack(0)
{
}

void Storage::Cluster::setTrack(Storage::Track* track)
{
  assert(!m_track && "Cluster: can't use a cluster for more than one track");
  m_track = track;
}

void Storage::Cluster::setMatchedTrack(Storage::Track* track)
{
  m_matchedTrack = track;
}

void Storage::Cluster::addHit(Storage::Hit* hit)
{
  if (m_hits.empty() == 0)
    m_timing = hit->getTiming();
  hit->setCluster(this);
  m_hits.push_back(hit);
  // Fill the value and timing from this hit
  m_value += hit->getValue();
}

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
