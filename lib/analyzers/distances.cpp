#include "distances.h"

#include <ostream>

#include <TDirectory.h>
#include <TH1D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Distances::Hists::Hists(TDirectory* dir,
                                   const std::string& prefix,
                                   const double distMax,
                                   const double d2Max,
                                   const int bins)
{
  using namespace Utils;

  HistAxis axU(-distMax, distMax, bins, "Cluster - track position u");
  HistAxis axV(-distMax, distMax, bins, "Cluster - track position v");
  HistAxis axDist(0, distMax, bins, "Cluster to track absolute distance");

  deltaU = makeH1(dir, prefix + "delta_u", axU);
  deltaV = makeH1(dir, prefix + "delta_v", axV);
  dist = makeH1(dir, prefix + "dist", axDist);
  if (0 < d2Max) {
    HistAxis axD2(0, d2Max, bins, "Cluster to track weighted squared distance");
    d2 = makeH1(dir, prefix + "d2", axD2);
  } else {
    d2 = nullptr;
  }
}

void Analyzers::Distances::Hists::fill(const Vector2& delta)
{
  deltaU->Fill(delta[0]);
  deltaV->Fill(delta[1]);
  dist->Fill(std::hypot(delta[0], delta[1]));
}

void Analyzers::Distances::Hists::fill(const Vector2& delta,
                                       const SymMatrix2& cov)
{
  fill(delta);
  if (d2) {
    d2->Fill(mahalanobisSquared(cov, delta));
  }
}

Analyzers::Distances::Distances(TDirectory* dir,
                                const Mechanics::Sensor& sensor,
                                const double pixelRange,
                                const double d2Max,
                                const int bins)
    : m_sensorId(sensor.id())
{
  auto area = sensor.sensitiveAreaLocal();
  double distMax = std::hypot(area.length(0), area.length(1));

  TDirectory* subDist =
      Utils::makeDir(dir, "sensors/" + sensor.name() + "/distances");
  m_trackTrack = Hists(subDist, "track_track-", distMax, -1, bins);
  m_trackCluster = Hists(subDist, "track_cluster-", distMax, d2Max, bins);
  m_clusterCluster = Hists(subDist, "cluster_cluster-", distMax, -1, bins);
}

std::string Analyzers::Distances::name() const
{
  return "Distances(" + std::to_string(m_sensorId) + ')';
}

void Analyzers::Distances::execute(const Storage::Event& event)
{
  const Storage::SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  // track-track and cluster-cluster distances are double-counted on purpose
  // to avoid biasing the resulting distributions due to position-dependent
  // ordering of the input container, e.g. clusters sorted by col-index

  // combinatorics: all tracks to all other tracks
  for (const auto& s0 : sensorEvent.localStates()) {
    for (const auto& s1 : sensorEvent.localStates()) {
      if (s0.first == s1.first) {
        continue;
      }
      m_trackTrack.fill(s1.second.offset() - s0.second.offset());
    }
  }
  // combinatorics: all clusters to all tracks
  for (const auto& s : sensorEvent.localStates()) {
    const Storage::TrackState& state = s.second;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = sensorEvent.getCluster(icluster);

      m_trackCluster.fill(cluster.posLocal() - state.offset(),
                          cluster.covLocal() + state.covOffset());
    }
  }
  // combinatorics: all clusters to all other clusters
  for (Index i0 = 0; i0 < sensorEvent.numClusters(); ++i0) {
    const auto& c0 = sensorEvent.getCluster(i0);

    for (Index i1 = 0; i1 < sensorEvent.numClusters(); ++i1) {
      if (i0 == i1) {
        continue;
      }
      const auto& c1 = sensorEvent.getCluster(i1);

      m_clusterCluster.fill(c1.posLocal() - c0.posLocal());
    }
  }
}
