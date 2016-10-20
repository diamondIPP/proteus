#include "cluster.h"

#include <cassert>
#include <ostream>

#include <Math/SMatrix.h>
#include <Math/SMatrixDfwd.h>
#include <Math/SVector.h>

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
  using Matrix3 = ROOT::Math::SMatrix<double, 3, 3>;
  using Matrix34 = ROOT::Math::SMatrix<double, 3, 4>;
  using SymMatrix3 =
      ROOT::Math::SMatrix<double, 3, 3, ROOT::Math::MatRepSym<double, 3>>;

  Matrix34 transform;
  SymMatrix3 covPixel, covGlobal;

  pixelToGlobal.GetTransformMatrix(transform);
  covPixel = ROOT::Math::SMatrixIdentity();
  covPixel(0, 0) = m_errCr.x() * m_errCr.x();
  covPixel(1, 1) = m_errCr.y() * m_errCr.y();
  covPixel(2, 2) = 1;
  covGlobal = Similarity(transform.Sub<Matrix3>(0, 0), covPixel);

  m_xyz = pixelToGlobal * XYZPoint(m_cr.x(), m_cr.y(), 0);
  m_errXyz.SetXYZ(std::sqrt(covGlobal(0, 0)),
                  std::sqrt(covGlobal(1, 1)),
                  std::sqrt(covGlobal(2, 2)));
}

Index Storage::Cluster::sensorId() const { return m_plane->sensorId(); }

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

std::ostream& Storage::operator<<(std::ostream& os, const Cluster& cluster)
{
  os << "cr=" << cluster.posPixel() << " size=" << cluster.numHits();
  return os;
}
