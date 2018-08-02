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

void Analyzers::Distances::Hists::fill(const XYVector& delta)
{
  deltaU->Fill(delta.x());
  deltaV->Fill(delta.y());
  dist->Fill(delta.r());
}

void Analyzers::Distances::Hists::fill(const XYVector& delta,
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
  double trackMax = std::hypot(area.length(0), area.length(1));

  TDirectory* subDist =
      Utils::makeDir(dir, "sensors/" + sensor.name() + "/distances");
  m_trackTrack = Hists(subDist, "track_track-", trackMax, -1, bins);
  m_trackCluster = Hists(subDist, "track_cluster-", trackMax, d2Max, bins);
}

std::string Analyzers::Distances::name() const
{
  return "Distances(" + std::to_string(m_sensorId) + ')';
}

void Analyzers::Distances::analyze(const Storage::Event& event)
{
  const Storage::SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  // combinatorics: all tracks to all other tracks
  for (const auto& s0 : sensorEvent.localStates()) {
    for (const auto& s1 : sensorEvent.localStates()) {
      if (s0.first == s1.first)
        continue;
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
}

void Analyzers::Distances::finalize()
{
  // nothing to do
}
