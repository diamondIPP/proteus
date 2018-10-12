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
    : m_col(std::numeric_limits<float>::quiet_NaN())
    , m_row(std::numeric_limits<float>::quiet_NaN())
    , m_u(std::numeric_limits<float>::quiet_NaN())
    , m_v(std::numeric_limits<float>::quiet_NaN())
    , m_time(std::numeric_limits<float>::quiet_NaN())
    , m_value(std::numeric_limits<float>::quiet_NaN())
    , m_crCov(SymMatrix2::Zero())
    , m_uvCov(SymMatrix2::Zero())
    , m_index(kInvalidIndex)
    , m_track(kInvalidIndex)
    , m_matchedState(kInvalidIndex)
{
}

void Storage::Cluster::setPixel(const Vector2& cr, const SymMatrix2& cov)
{
  m_col = cr[0];
  m_row = cr[1];
  m_crCov = cov.selfadjointView<Eigen::Lower>();
}

void Storage::Cluster::setLocal(const Vector2& uv, const SymMatrix2& cov)
{
  m_u = uv[0];
  m_v = uv[1];
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

int Storage::Cluster::sizeCol() const { return areaPixel().length(0); }
int Storage::Cluster::sizeRow() const { return areaPixel().length(1); }

void Storage::Cluster::addHit(Storage::Hit& hit)
{
  hit.setCluster(m_index);
  m_hits.push_back(std::ref(hit));
}

void Storage::Cluster::print(std::ostream& os, const std::string& prefix) const
{
  auto stdevCR = extractStdev(m_crCov);
  auto stdevUV = extractStdev(m_uvCov);
  os << prefix << "size: " << size() << '\n';
  os << prefix << "pixel: [" << m_col << ", " << m_row << "]\n";
  os << prefix << "pixel stdev: [" << stdevCR[0] << ", " << stdevCR[1] << "]\n";
  os << prefix << "local: [" << m_u << ", " << m_v << "]\n";
  os << prefix << "local stdev: [" << stdevUV[0] << ", " << stdevUV[1] << "]\n";
  os.flush();
}

std::ostream& Storage::operator<<(std::ostream& os, const Cluster& cluster)
{
  auto cr = cluster.posPixel();
  auto uv = cluster.posLocal();
  os << "size=" << cluster.size();
  os << " pixel=[" << cr[0] << "," << cr[1] << "]";
  os << " local=[" << uv[0] << "," << uv[1] << "]";
  if (cluster.isInTrack()) {
    os << " track=" << cluster.track();
  }
  if (cluster.isMatched()) {
    os << " matched=" << cluster.matchedState();
  }
  return os;
}
