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
    : m_time(-1)
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

  SymMatrix3 covPixel;
  covPixel.Place_at(m_covCr, 0, 0);
  // avoid singular matrix by setting local w-error to sensor thickness
  covPixel(2, 2) = 1.0 / 12.0;

  m_xyz = pixelToGlobal * XYZPoint(m_cr.x(), m_cr.y(), 0);
  m_covXyz = Similarity(transform.Sub<Matrix3>(0, 0), covPixel);
}

Index Storage::Cluster::sensorId() const { return m_plane->sensorId(); }

Storage::Cluster::Area Storage::Cluster::area() const
{
  if (m_hits.empty()) {
    // empty area anchored outside the sensitive sensor area
    return Area(Area::Axis(-1, -1), Area::Axis(-1, -1));
  }

  Area::Axis col(m_hits.front()->col(), m_hits.front()->col() + 1);
  Area::Axis row(m_hits.front()->row(), m_hits.front()->row() + 1);
  for (auto hit = ++m_hits.begin(); hit != m_hits.end(); ++hit) {
    col.min = std::min(col.min, static_cast<int>((*hit)->col()));
    col.max = std::max(col.max, static_cast<int>((*hit)->col()) + 1);
    row.min = std::min(row.min, static_cast<int>((*hit)->row()));
    row.max = std::max(row.max, static_cast<int>((*hit)->row()) + 1);
  }
  return Area(col, row);
}

int Storage::Cluster::sizeCol() const { return area().axes[0].length(); }
int Storage::Cluster::sizeRow() const { return area().axes[1].length(); }

void Storage::Cluster::setTrack(const Storage::Track* track)
{
  assert(!m_track && "Cluster: can't use a cluster for more than one track");
  m_track = track;
}

void Storage::Cluster::addHit(Storage::Hit* hit)
{
  if (m_hits.empty() == 0)
    m_time = hit->time();
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
