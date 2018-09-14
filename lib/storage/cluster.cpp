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
    : m_cr(std::numeric_limits<double>::quiet_NaN(),
           std::numeric_limits<double>::quiet_NaN())
    , m_uv(std::numeric_limits<double>::quiet_NaN(),
           std::numeric_limits<double>::quiet_NaN())
    , m_time(std::numeric_limits<float>::quiet_NaN())
    , m_value(std::numeric_limits<float>::quiet_NaN())
    , m_index(kInvalidIndex)
    , m_track(kInvalidIndex)
    , m_matchedState(kInvalidIndex)
{
}

void Storage::Cluster::setPixel(const Vector2& cr, const SymMatrix2& cov)
{
  m_cr = cr;
  m_crCov = cov;
}

void Storage::Cluster::setPixel(float col,
                                float row,
                                float stdCol,
                                float stdRow)
{
  m_cr[0] = col;
  m_cr[1] = row;
  m_crCov(0, 0) = stdCol * stdCol;
  m_crCov(1, 1) = stdRow * stdRow;
  m_crCov(0, 1) = m_crCov(1, 0) = 0;
}

void Storage::Cluster::setLocal(const Vector2& uv, const SymMatrix2& cov)
{
  m_uv = uv;
  m_uvCov = cov;
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
  Vector2 ep = sqrt(m_crCov.Diagonal());
  Vector2 el = sqrt(m_uvCov.Diagonal());

  os << prefix << "size: " << size() << '\n';
  os << prefix << "pixel: " << posPixel() << '\n';
  os << prefix << "pixel stddev: " << ep << '\n';
  os << prefix << "local: " << posLocal() << '\n';
  os << prefix << "local stddev: " << el << '\n';
  os.flush();
}

std::ostream& Storage::operator<<(std::ostream& os, const Cluster& cluster)
{
  auto cr = cluster.posPixel();
  auto uv = cluster.posLocal();
  os << "size=" << cluster.size();
  os << " pixel=[" << cr[0] << "," << cr[1] << "]";
  os << " local=[" << uv[0] << "," << uv[1] << "]";
  if (cluster.isInTrack())
    os << " track=" << cluster.track();
  if (cluster.isMatched())
    os << " matched=" << cluster.track();
  return os;
}
