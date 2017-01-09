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

  deltaU = makeH1(dir, prefix + "DeltaU", axU);
  deltaV = makeH1(dir, prefix + "DeltaV", axV);
  dist = makeH1(dir, prefix + "Dist", axDist);
  if (0 < d2Max) {
    HistAxis axD2(0, d2Max, bins, "Cluster to track weighted squared distance");
    d2 = makeH1(dir, prefix + "D2", axD2);
  } else {
    d2 = NULL;
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
  d2->Fill(mahalanobisSquared(cov, delta));
}

Analyzers::Distances::Distances(const Mechanics::Device& device,
                                Index sensorId,
                                TDirectory* dir,
                                const double pixelRange,
                                const double d2Max,
                                const int bins)
    : m_sensorId(sensorId)
{
  const Mechanics::Sensor& sensor = *device.getSensor(sensorId);
  auto area = sensor.sensitiveAreaLocal();
  double pitch = std::hypot(sensor.pitchCol(), sensor.pitchRow());
  double trackMax = std::hypot(area.length(0), area.length(1));
  double matchMax = pixelRange * pitch;

  TDirectory* sub = Utils::makeDir(dir, "Distances");

  m_trackTrack = Hists(sub, sensor.name() + "-TrackTrack-", trackMax, -1, bins);
  m_trackCluster =
      Hists(sub, sensor.name() + "-TrackCluster-", trackMax, d2Max, bins);
  m_match = Hists(sub, sensor.name() + "-Match-", matchMax, d2Max, bins);
}

std::string Analyzers::Distances::name() const
{
  return "Distances(" + std::to_string(m_sensorId) + ')';
}

void Analyzers::Distances::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  // combinatorics: all tracks to all other tracks
  for (Index istate0 = 0; istate0 < plane.numStates(); ++istate0) {
    for (Index istate1 = 0; istate1 < plane.numStates(); ++istate1) {
      if (istate0 == istate1)
        continue;
      m_trackTrack.fill(plane.getState(istate1).offset() -
                        plane.getState(istate0).offset());
    }
  }
  // combinatorics: all clusters to all tracks
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      const Storage::TrackState& state = plane.getState(istate);
      const Storage::Cluster& cluster = *plane.getCluster(icluster);
      m_trackCluster.fill(cluster.posLocal() - state.offset(),
                          cluster.covLocal() + state.covOffset());
    }
  }
  // matched pairs
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    const Storage::TrackState& state = plane.getState(istate);
    if (state.matchedCluster()) {
      m_match.fill(state.matchedCluster()->posLocal() - state.offset(),
                   state.matchedCluster()->covLocal() + state.covOffset());
    }
  }
}

void Analyzers::Distances::finalize()
{
  // nothing to do
}