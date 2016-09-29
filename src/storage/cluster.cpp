#include "cluster.h"

#include <cassert>
#include <ostream>

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

void Storage::Cluster::transformToGlobal(const Transform3D& pixelToGlobal)
{
  m_xyz = pixelToGlobal * XYZPoint(m_cr.x(), m_cr.y(), 0);
  m_errXyz =  pixelToGlobal * XYZVector(m_errCr.x(), m_errCr.y(), 0);
  // poor mans solution; use full covariance and jacobian
  m_errXyz.SetX(std::abs(m_errXyz.x()));
  m_errXyz.SetY(std::abs(m_errXyz.y()));
  m_errXyz.SetZ(std::abs(m_errXyz.y()));
}

void Storage::Cluster::setTrack(Storage::Track* track)
{
  assert(!m_track && "Cluster: can't use a cluster for more than one track");
  m_track = track;
}

void Storage::Cluster::addHit(Storage::Hit* hit)
{
  if (m_hits.empty() == 0)
    m_timing = hit->timing();
  hit->setCluster(this);
  m_hits.push_back(hit);
  // Fill the value and timing from this hit
  m_value += hit->value();
}

void Storage::Cluster::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "pixel: " << posPixel() << '\n';
  os << prefix << "pixel error: " << errPixel() << '\n';
  os << prefix << "global: " << posGlobal() << '\n';
  os << prefix << "global error: " << errGlobal() << '\n';
  os << prefix << "size: " << getNumHits() << '\n';
  os.flush();
}
