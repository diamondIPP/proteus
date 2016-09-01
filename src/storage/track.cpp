#include "track.h"

#include "cluster.h"

Storage::Track::Track()
    : m_originX(0)
    , m_originY(0)
    , m_originErrX(0)
    , m_originErrY(0)
    , m_slopeX(0)
    , m_slopeY(0)
    , m_slopeErrX(0)
    , m_slopeErrY(0)
    , m_covarianceX(0)
    , m_covarianceY(0)
    , m_chi2(0)
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
