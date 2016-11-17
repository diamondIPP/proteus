#include "cluster.h"

#include <cassert>
#include <climits>
#include <ostream>

#include "storage/hit.h"
#include "storage/plane.h"
#include "storage/track.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Cluster)

Storage::Cluster::Cluster()
    : m_timing(-1)
    , m_value(-1)
    , m_plane(NULL)
    , m_track(NULL)
    , m_matchedTrack(NULL)
    , m_matchDistance(-1)
{
}

void Storage::Cluster::setErrPixel(double stdCol, double stdRow)
{
  m_covCr(0, 0) = stdCol * stdCol;
  m_covCr(0, 1) = 0;
  m_covCr(1, 1) = stdRow * stdRow;
}

void Storage::Cluster::transformToGlobal(const Transform3D& pixelToGlobal)
{
  Matrix34 transform;
  pixelToGlobal.GetTransformMatrix(transform);

  m_xyz = pixelToGlobal * XYZPoint(m_cr.x(), m_cr.y(), 0);
  m_covXyz = Similarity(transform.Sub<Matrix32>(0, 0), m_covCr);
}

Index Storage::Cluster::sensorId() const { return m_plane->sensorId(); }

int Storage::Cluster::sizeCol() const
{
  if (m_hits.empty())
    return 0;
  int imin = INT_MAX;
  int imax = INT_MIN;
  for (auto hit = m_hits.begin(); hit != m_hits.end(); ++hit) {
    imin = std::min<int>(imin, (*hit)->col());
    imax = std::max<int>(imax, (*hit)->col());
  }
  return imax - imin + 1;
}

int Storage::Cluster::sizeRow() const
{
  if (m_hits.empty())
    return 0;
  int imin = INT_MAX;
  int imax = INT_MIN;
  for (auto hit = m_hits.begin(); hit != m_hits.end(); ++hit) {
    imin = std::min<int>(imin, (*hit)->row());
    imax = std::max<int>(imax, (*hit)->row());
  }
  return imax - imin + 1;
}

void Storage::Cluster::setTrack(const Storage::Track* track)
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
  Vector2 ep = sqrt(m_covCr.Diagonal());
  Vector3 eg = sqrt(m_covXyz.Diagonal());

  os << prefix << "pixel: " << posPixel() << '\n';
  os << prefix << "pixel stddev: " << ep << '\n';
  os << prefix << "global: " << posGlobal() << '\n';
  os << prefix << "global stddev: " << eg << '\n';
  os << prefix << "size: " << getNumHits() << '\n';
  os.flush();
}

std::ostream& Storage::operator<<(std::ostream& os, const Cluster& cluster)
{
  os << "pixel=" << cluster.posPixel() << " size=" << cluster.numHits();
  return os;
}
