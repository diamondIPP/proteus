#include "track.h"

#include <ostream>

#include <Math/ProbFunc.h>

#include "cluster.h"

Storage::Track::Track() : m_chi2(-1), m_dof(-1) {}

Storage::Track::Track(const TrackState& global, Scalar chi2, int dof)
    : m_state(global), m_chi2(chi2), m_dof(dof)
{
}

Scalar Storage::Track::probability() const
{
  return ((0 < m_dof) and (0 <= m_chi2))
             ? ROOT::Math::chisquared_cdf_c(m_chi2, m_dof)
             : std::numeric_limits<Scalar>::quiet_NaN();
}

// NOTE: this doesn't tell the cluster about the track
void Storage::Track::addCluster(Index sensor, Cluster& cluster)
{
  m_clusters.emplace(sensor, cluster);
}

void Storage::Track::freezeClusterAssociation(Index trackIdx)
{
  for (const auto& c : m_clusters) {
    c.second.get().setTrack(trackIdx);
  }
}

std::ostream& Storage::operator<<(std::ostream& os, const Storage::Track& track)
{
  os << "chi2/dof=" << track.chi2() << "/" << track.degreesOfFreedom();
  os << " prob=" << track.probability();
  os << " size=" << track.size();
  return os;
}
