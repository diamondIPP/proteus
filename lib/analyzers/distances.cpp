#include "distances.h"

#include <ostream>

#include <TDirectory.h>
#include <TH1D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

namespace proteus {

Distances::Distances(TDirectory* dir, const Sensor& sensor)
    : m_sensorId(sensor.id())
{
  TDirectory* subDir = makeDir(dir, "sensors/" + sensor.name() + "/distances");

  auto makeDeltaHist = [&](int dim, std::string name, std::string label) {
    auto iv = sensor.sensitiveVolume().interval(dim);
    auto pitch = sensor.pitch()[dim];
    auto axis = HistAxis::Difference(iv, pitch, std::move(label));
    return makeH1(subDir, name, axis);
  };
  auto makeHists = [&](std::string name, std::string label) {
    auto box = sensor.sensitiveVolume();
    auto pitch = sensor.pitch();
    auto distMax = std::hypot(box.length(kU), box.length(kV));
    int distBins = std::round(distMax / std::min(pitch[kU], pitch[kV]));
    auto distAxis = HistAxis{0, distMax, distBins, label + "absolute distance"};

    Hists h;
    h.deltaU = makeDeltaHist(kU, name + "delta_u", label + "position u");
    h.deltaV = makeDeltaHist(kV, name + "delta_v", label + "position v");
    h.deltaS = makeDeltaHist(kS, name + "delta_time", label + "local time");
    h.dist = makeH1(subDir, name + "dist", distAxis);
    return h;
  };

  m_trackTrack = makeHists("track_track-", "Track - track ");
  m_trackCluster = makeHists("track_cluster-", "Cluster - track ");
  m_clusterCluster = makeHists("cluster_cluster-", "Cluster - cluster ");
}

std::string Distances::name() const
{
  return "Distances(" + std::to_string(m_sensorId) + ')';
}

void Distances::execute(const Event& event)
{
  const SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  auto fillDelta = [](Hists& hs, const Vector4& delta) {
    hs.deltaU->Fill(delta[kU]);
    hs.deltaV->Fill(delta[kV]);
    hs.deltaS->Fill(delta[kS]);
    hs.dist->Fill(std::hypot(delta[kU], delta[kV]));
  };

  // track-track and cluster-cluster distances are double-counted on purpose
  // to avoid biasing the resulting distributions due to position-dependent
  // ordering of the input container, e.g. clusters sorted by col-index

  // combinatorics: all tracks to all other tracks
  for (const auto& s0 : sensorEvent.localStates()) {
    for (const auto& s1 : sensorEvent.localStates()) {
      if (s0.track() == s1.track()) {
        continue;
      }

      fillDelta(m_trackTrack, s1.position() - s0.position());
    }
  }
  // combinatorics: all clusters to all tracks
  for (const auto& state : sensorEvent.localStates()) {
    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      const Cluster& cluster = sensorEvent.getCluster(icluster);

      fillDelta(m_trackCluster, cluster.position() - state.position());
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

      fillDelta(m_clusterCluster, c1.position() - c0.position());
    }
  }
}

} // namespace proteus
