// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "track.h"

#include <ostream>
#include <stdexcept>

#include <Math/ProbFunc.h>

namespace proteus {

Track::Track(const TrackState& global, Scalar chi2, int dof)
    : m_state(global), m_chi2(chi2), m_dof(dof)
{
}

Scalar Track::probability() const
{
  return ((0 < m_dof) and (0 <= m_chi2))
             ? ROOT::Math::chisquared_cdf_c(m_chi2, m_dof)
             : std::numeric_limits<Scalar>::quiet_NaN();
}

namespace {
/** Helper functor struct to find a track cluster on a specific sensor. */
struct OnSensor {
  Index sensorId;

  bool operator()(const Track::TrackCluster& tc) const
  {
    return tc.sensor == sensorId;
  }
};
} // namespace

void Track::addCluster(Index sensor, Index cluster)
{
  auto c = std::find_if(m_clusters.begin(), m_clusters.end(), OnSensor{sensor});
  if (c != m_clusters.end()) {
    c->cluster = cluster;
  } else {
    m_clusters.push_back({sensor, cluster});
  }
}

bool Track::hasClusterOn(Index sensor) const
{
  auto c = std::find_if(m_clusters.begin(), m_clusters.end(), OnSensor{sensor});
  return (c != m_clusters.end());
}

Index Track::getClusterOn(Index sensor) const
{
  auto c = std::find_if(m_clusters.begin(), m_clusters.end(), OnSensor{sensor});
  if (c == m_clusters.end()) {
    throw std::out_of_range("No cluster exists on requested sensor");
  }
  return c->cluster;
}

std::ostream& operator<<(std::ostream& os, const Track& track)
{
  os << "chi2/dof=" << track.chi2() << "/" << track.degreesOfFreedom();
  os << " prob=" << track.probability();
  os << " size=" << track.size();
  return os;
}

} // namespace proteus
