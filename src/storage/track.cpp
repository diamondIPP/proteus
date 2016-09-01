#include "track.h"

#include <ostream>

#include "cluster.h"

Storage::TrackState::TrackState(double posU,
                                double posV,
                                double slopeU,
                                double slopeV)
    : m_u(posU)
    , m_v(posV)
    , m_du(slopeU)
    , m_dv(slopeV)
    , m_errU(0)
    , m_errDu(0)
    , m_covUDu(0)
    , m_errV(0)
    , m_errDv(0)
    , m_covVDv(0)
{
}

Storage::TrackState::TrackState(const XYPoint& offset, const XYVector& slope)
    : TrackState(offset.x(), offset.y(), slope.x(), slope.y())
{
}

void Storage::TrackState::setOffsetErr(double errU, double errV)
{
  m_errU = errU;
  m_errV = errV;
}

void Storage::TrackState::setErrU(double errU, double errDu, double cov)
{
  m_errU = errU;
  m_errDu = errDu;
  m_covUDu = cov;
}

void Storage::TrackState::setErrV(double errV, double errDv, double cov)
{
  m_errV = errV;
  m_errDv = errDv;
  m_covVDv = cov;
}

Storage::Track::Track()
    : m_state(0, 0, 0, 0)
    , m_chi2(-1)
    , m_index(-1)
{
}

Storage::Track::Track(const TrackState& globalState)
    : m_state(globalState)
    , m_chi2(-1)
    , m_index(-1)
{
}

// NOTE: this doesn't tell the cluster about the track (due to building trial
// tracks)
void Storage::Track::addCluster(Cluster* cluster)
{
  m_clusters.push_back(cluster);
}

void Storage::Track::addMatchedCluster(Cluster* cluster)
{
  m_matchedClusters.push_back(cluster);
}

void Storage::TrackState::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "offset: " << offset() << '\n';
  os << prefix << "slope: " << slope() << '\n';
}

void Storage::Track::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "chi2: " << m_chi2 << '\n';
  os << prefix << "num clusters: " << m_clusters.size() << '\n';
  os << prefix << "global state:\n";
  m_state.print(os, prefix + "  ");
}
