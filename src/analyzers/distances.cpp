#include "distances.h"

#include <ostream>

#include <TDirectory.h>
#include <TH1D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Distances::Hists::Hists(TDirectory* dir,
                                   std::string prefix,
                                   double rangeDist,
                                   double rangeD2,
                                   int numBins)
{
  auto h1 = [&](std::string name, double x0, double x1) -> TH1D* {
    TH1D* h = new TH1D((prefix + name).c_str(), "", numBins, x0, x1);
    h->SetDirectory(dir);
    return h;
  };
  deltaU = h1("DeltaU", -rangeDist, rangeDist);
  deltaV = h1("DeltaV", -rangeDist, rangeDist);
  dist = h1("Dist", 0, rangeDist);
  d2 = (0 < rangeD2) ? h1("D2", 0, rangeD2) : NULL;
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
                                TDirectory* dir)
    : m_sensorId(sensorId)
{
  const Mechanics::Sensor& sensor = *device.getSensor(sensorId);
  auto area = sensor.sensitiveAreaLocal();
  double rangeTrack = std::hypot(area.axes[0].length(), area.axes[1].length());
  double rangeDist = std::hypot(sensor.pitchCol(), sensor.pitchRow());
  double rangeD2 = 10;
  int numBins = 256;
  TDirectory* sub = Utils::makeDir(dir, "Distances");

  m_trackTrack =
      Hists(sub, sensor.name() + "-TrackTrack-", rangeTrack, -1, numBins);
  m_trackCluster = Hists(sub, sensor.name() + "-TrackCluster-", 4 * rangeDist,
                         rangeD2, numBins);
  m_match =
      Hists(sub, sensor.name() + "-Match-", 1.5 * rangeDist, rangeD2, numBins);
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
