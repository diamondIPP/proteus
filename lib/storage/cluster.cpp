#include "cluster.h"

#include <cassert>
#include <climits>
#include <limits>
#include <numeric>
#include <ostream>

#include "storage/hit.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Cluster)

Storage::Cluster::Cluster()
    : m_cr(std::numeric_limits<Scalar>::quiet_NaN(),
           std::numeric_limits<Scalar>::quiet_NaN())
    , m_uv(std::numeric_limits<Scalar>::quiet_NaN(),
           std::numeric_limits<Scalar>::quiet_NaN())
    , m_time(std::numeric_limits<Scalar>::quiet_NaN())
    , m_value(std::numeric_limits<Scalar>::quiet_NaN())
    , m_crCov(SymMatrix2::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_uvCov(SymMatrix2::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_index(kInvalidIndex)
    , m_track(kInvalidIndex)
    , m_matchedState(kInvalidIndex)
{
}

void Storage::Cluster::setPixel(const Vector2& cr, const SymMatrix2& cov)
{
  m_cr = cr;
  m_crCov = cov.selfadjointView<Eigen::Lower>();
}

void Storage::Cluster::setLocal(const Vector2& uv, const SymMatrix2& cov)
{
  m_uv = uv;
  m_uvCov = cov.selfadjointView<Eigen::Lower>();
}

bool Storage::Cluster::hasRegion() const
{
  return !m_hits.empty() && m_hits.front().get().hasRegion();
}

Index Storage::Cluster::region() const
{
  return m_hits.empty() ? kInvalidIndex : m_hits.front().get().region();
}

void Storage::Cluster::setTrack(Index track)
{
  assert((m_track == kInvalidIndex) && "cluster can only be in one track");
  m_track = track;
}

Storage::Cluster::Area Storage::Cluster::areaPixel() const
{
  auto grow = [](Area a, const Hit& hit) {
    a.enclose(hit.areaPixel());
    return a;
  };
  return std::accumulate(m_hits.begin(), m_hits.end(), Area::Empty(), grow);
}

void Storage::Cluster::addHit(Storage::Hit& hit)
{
  hit.setCluster(m_index);
  m_hits.push_back(std::ref(hit));
}

void Storage::Cluster::print(std::ostream& os, const std::string& prefix) const
{
  auto stdCR = extractStdev(m_crCov);
  auto stdUV = extractStdev(m_uvCov);
  os << prefix << "size: " << size() << '\n';
  os << prefix << "pixel: [" << m_cr[0] << ", " << m_cr[1] << "]\n";
  os << prefix << "pixel stdev: [" << stdCR[0] << ", " << stdCR[1] << "]\n";
  os << prefix << "local: [" << m_uv[0] << ", " << m_uv[1] << "]\n";
  os << prefix << "local stdev: [" << stdUV[0] << ", " << stdUV[1] << "]\n";
  os.flush();
}

std::ostream& Storage::operator<<(std::ostream& os, const Cluster& cluster)
{
  os << "size=" << cluster.size();
  os << " pixel=[" << cluster.posPixel().transpose() << "]";
  os << " local=[" << cluster.posLocal().transpose() << "]";
  if (cluster.isInTrack()) {
    os << " track=" << cluster.track();
  }
  if (cluster.isMatched()) {
    os << " matched=" << cluster.matchedState();
  }
  return os;
}
