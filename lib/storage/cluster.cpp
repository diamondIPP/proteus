// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "cluster.h"

#include <cassert>
#include <climits>
#include <limits>
#include <numeric>
#include <ostream>

#include "storage/hit.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Cluster)

namespace proteus {

Cluster::Cluster(Scalar col,
                 Scalar row,
                 Scalar timestamp,
                 Scalar value,
                 Scalar colVar,
                 Scalar rowVar,
                 Scalar timestampVar,
                 Scalar colRowCov)
    : m_col(col)
    , m_row(row)
    , m_timestamp(timestamp)
    , m_value(value)
    , m_colVar(colVar)
    , m_rowVar(rowVar)
    , m_colRowCov(colRowCov)
    , m_timestampVar(timestampVar)
    , m_pos(Vector4::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_posCov(SymMatrix4::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_index(kInvalidIndex)
    , m_track(kInvalidIndex)
    , m_matchedState(kInvalidIndex)
{
}

bool Cluster::hasRegion() const
{
  return !m_hits.empty() && m_hits.front().get().hasRegion();
}

Index Cluster::region() const
{
  return m_hits.empty() ? kInvalidIndex : m_hits.front().get().region();
}

void Cluster::setTrack(Index track)
{
  assert((m_track == kInvalidIndex) && "cluster can only be in one track");
  m_track = track;
}

static Matrix<Scalar, 3, 4> projectionOntoPlane()
{
  Matrix<Scalar, 3, 4> proj = Matrix<Scalar, 3, 4>::Zero();
  proj(kLoc0 - kOnPlane, kU) = 1;
  proj(kLoc1 - kOnPlane, kV) = 1;
  proj(kTime - kOnPlane, kS) = 1;
  return proj;
}

Vector3 Cluster::onPlane() const { return projectionOntoPlane() * m_pos; }

SymMatrix3 Cluster::onPlaneCov() const
{
  return transformCovariance(projectionOntoPlane(), m_posCov);
}

Cluster::Area Cluster::areaPixel() const
{
  auto grow = [](Area a, const Hit& hit) {
    a.enclose(Area(Area::AxisInterval(hit.col(), hit.col() + 1),
                   Area::AxisInterval(hit.row(), hit.row() + 1)));
    return a;
  };
  return std::accumulate(m_hits.begin(), m_hits.end(), Area::Empty(), grow);
}

void Cluster::addHit(Hit& hit)
{
  hit.setCluster(m_index);
  m_hits.push_back(std::ref(hit));
}

std::ostream& operator<<(std::ostream& os, const Cluster& cluster)
{
  os << "col=" << cluster.col();
  os << " row=" << cluster.row();
  os << " u=" << cluster.u();
  os << " v=" << cluster.v();
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

} // namespace proteus
