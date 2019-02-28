#include "track.h"

#include <ostream>

#include <Math/ProbFunc.h>

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

void Storage::Track::addCluster(Index sensor, Index cluster)
{
  auto pos = std::find_if(m_clusters.begin(), m_clusters.end(),
                          [=](const auto& c) { return c.sensor == sensor; });
  if (pos != m_clusters.end()) {
    pos->cluster = cluster;
  } else {
    m_clusters.push_back({sensor, cluster});
  }
}

std::ostream& Storage::operator<<(std::ostream& os, const Storage::Track& track)
{
  os << "chi2/dof=" << track.chi2() << "/" << track.degreesOfFreedom();
  os << " prob=" << track.probability();
  os << " size=" << track.size();
  return os;
}
