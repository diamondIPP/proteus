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

Storage::Cluster::Cluster()

    : m_timing(-1)
    , m_value(-1)
    , m_plane(NULL)
    , m_track(NULL)
    , m_matchedTrack(NULL)
    , m_matchDistance(-1)
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
