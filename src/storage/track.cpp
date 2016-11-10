#include "track.h"

#include <ostream>

#include "cluster.h"

Storage::TrackState::TrackState(double u, double v, double dU, double dV)
    : m_offset(u, v), m_slope(dU, dV)
{
}

Storage::TrackState::TrackState(const XYPoint& offset, const XYVector& slope)
    : TrackState(offset.x(), offset.y(), slope.x(), slope.y())
{
}

Storage::TrackState::TrackState() : TrackState(0, 0, 0, 0) {}

void Storage::TrackState::setCovU(double varOffset, double varSlope, double cov)
{
  m_cov(U, U) = varOffset;
  m_cov(U, V) = 0;
  m_cov(U, DU) = cov;
  m_cov(U, DV) = 0;
  m_cov(DU, V) = 0;
  m_cov(DU, DU) = varSlope;
  m_cov(DU, DV) = 0;
}

void Storage::TrackState::setCovV(double varOffset, double varSlope, double cov)
{
  m_cov(V, U) = 0;
  m_cov(V, V) = varOffset;
  m_cov(V, DU) = 0;
  m_cov(V, DV) = cov;
  m_cov(DV, U) = 0;
  m_cov(DV, DU) = 0;
  m_cov(DV, DV) = varSlope;
}

void Storage::TrackState::setErrU(double stdOffset, double stdSlope, double cov)
{
  setCovU(stdOffset * stdOffset, stdSlope * stdSlope, cov);
}

void Storage::TrackState::setErrV(double stdOffset, double stdSlope, double cov)
{
  setCovV(stdOffset * stdOffset, stdSlope * stdSlope, cov);
}

Storage::Track::Track() : m_state(0, 0, 0, 0), m_redChi2(-1), m_index(-1) {}

Storage::Track::Track(const TrackState& globalState)
    : m_state(globalState), m_redChi2(-1), m_index(-1)
{
}

void Storage::Track::addLocalState(Index sensor, const TrackState& state)
{
  m_localStates[sensor] = state;
}

bool Storage::Track::hasLocalState(Index sensor) const
{
  return (0 < m_localStates.count(sensor));
}

const Storage::TrackState& Storage::Track::localState(Index sensor) const
{
  return m_localStates.at(sensor);
}

// NOTE: this doesn't tell the cluster about the track (due to building trial
// tracks)
void Storage::Track::addCluster(Cluster* cluster)
{
  m_clusters.push_back(cluster);
}

void Storage::Track::fixClusterAssociation()
{
  for (auto cluster = m_clusters.begin(); cluster != m_clusters.end();
       ++cluster)
    (*cluster)->setTrack(this);
}

void Storage::Track::addMatchedCluster(Cluster* cluster)
{
  m_matchedClusters.push_back(cluster);
}

void Storage::TrackState::print(std::ostream& os,
                                const std::string& prefix) const
{
  os << prefix << "offset: " << offset() << '\n';
  os << prefix << "slope: " << slope() << '\n';
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "uv=" << state.offset() << " d(uv)=" << state.slope();
  return os;
}

void Storage::Track::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "chi2/ndf: " << m_redChi2 << '\n';
  os << prefix << "points: " << m_clusters.size() << '\n';
  os << prefix << "global: " << m_state << '\n';
  if (!m_localStates.empty()) {
    for (auto is = m_localStates.begin(); is != m_localStates.end(); ++is) {
      const auto& sensorId = is->first;
      const auto& state = is->second;
      os << prefix << "  on sensor " << sensorId << ": " << state << '\n';
    }
  }
}
