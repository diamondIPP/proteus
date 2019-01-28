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
    : m_col(std::numeric_limits<Scalar>::quiet_NaN())
    , m_row(std::numeric_limits<Scalar>::quiet_NaN())
    , m_timestamp(std::numeric_limits<Scalar>::quiet_NaN())
    , m_value(std::numeric_limits<Scalar>::quiet_NaN())
    , m_colVar(0)
    , m_rowVar(0)
    , m_colRowCov(0)
    , m_timestampVar(0)
    , m_pos(Vector4::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_posCov(SymMatrix4::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_index(kInvalidIndex)
    , m_track(kInvalidIndex)
    , m_matchedState(kInvalidIndex)
{
}

void Storage::Cluster::setPixel(const Vector2& cr,
                                const SymMatrix2& crCov,
                                Scalar timestamp,
                                Scalar timestampVar)
{
  m_col = cr[0];
  m_row = cr[1];
  m_timestamp = timestamp;
  m_colVar = crCov(0, 0);
  m_rowVar = crCov(1, 1);
  m_colRowCov = crCov(1, 0);
  m_timestampVar = timestampVar;
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
    a.enclose(Area(Area::AxisInterval(hit.col(), hit.col() + 1),
                   Area::AxisInterval(hit.row(), hit.row() + 1)));
    return a;
  };
  return std::accumulate(m_hits.begin(), m_hits.end(), Area::Empty(), grow);
}

void Storage::Cluster::addHit(Storage::Hit& hit)
{
  hit.setCluster(m_index);
  m_hits.push_back(std::ref(hit));
}

std::ostream& Storage::operator<<(std::ostream& os, const Cluster& cluster)
{
  os << "col=" << cluster.col();
  os << " row=" << cluster.row();
  os << " loc=" << format(cluster.location());
  os << " time=" << cluster.time();
  os << " value=" << cluster.value();
  os << " size=" << cluster.size();
  if (cluster.isInTrack()) {
    os << " track=" << cluster.track();
  }
  if (cluster.isMatched()) {
    os << " matched=" << cluster.matchedState();
  }
  return os;
}
