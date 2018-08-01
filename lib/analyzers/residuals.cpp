#include "residuals.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_GLOBAL_LOGGER

Analyzers::detail::SensorResidualHists::SensorResidualHists(
    TDirectory* dir,
    const Mechanics::Sensor& sensor,
    const double rangeStd,
    const int bins,
    const std::string& name)
{
  using namespace Utils;

  auto resMaxU = rangeStd * sensor.pitchCol() / std::sqrt(12.0);
  auto resMaxV = rangeStd * sensor.pitchRow() / std::sqrt(12.0);
  auto posRangeU = sensor.sensitiveAreaLocal().interval(0);
  auto posRangeV = sensor.sensitiveAreaLocal().interval(1);
  auto distRange = std::sqrt(resMaxU*resMaxU + resMaxV*resMaxV);

  Vector2 slope = sensor.beamSlope();
  Vector2 slopeMin = slope - rangeStd * sensor.beamDivergence();
  Vector2 slopeMax = slope + rangeStd * sensor.beamDivergence();

  HistAxis axResU(-resMaxU, resMaxU, bins, "Cluster - track residual u");
  HistAxis axResV(-resMaxV, resMaxV, bins, "Cluster - track residual v");
  HistAxis axPosU(posRangeU, bins, "Local track position u");
  HistAxis axPosV(posRangeV, bins, "Local track position v");
  HistAxis axSlopeU(slopeMin[0], slopeMax[0], bins, "Local track slope u");
  HistAxis axSlopeV(slopeMin[1], slopeMax[1], bins, "Local track slope v");
  HistAxis axResDist(0, distRange, bins, "Cluster - track distance");
  HistAxis axResD2(0, 2*rangeStd, bins, "Cluster - track weighted squared distance");

  TDirectory* sub = makeDir(dir, "sensors/" + sensor.name() + "/" + name);
  resU = makeH1(sub, "res_u", axResU);
  trackUResU = makeH2(sub, "res_u-pos_u", axPosU, axResU);
  trackVResU = makeH2(sub, "res_u-pos_v", axPosV, axResU);
  slopeUResU = makeH2(sub, "res_u-slope_u", axSlopeU, axResU);
  slopeVResU = makeH2(sub, "res_u-slope_v", axSlopeV, axResU);

  resV = makeH1(sub, "res_v", axResV);
  trackUResV = makeH2(sub, "res_v-pos_u", axPosU, axResV);
  trackVResV = makeH2(sub, "res_v-pos_v", axPosV, axResV);
  slopeUResV = makeH2(sub, "res_v-slope_u", axSlopeU, axResV);
  slopeVResV = makeH2(sub, "res_v-slope_v", axSlopeV, axResV);

  resUV = makeH2(sub, "res_uv", axResU, axResV);

  resDist = makeH1(sub, "res_dist", axResDist);
  resD2 = makeH1(sub, "res_d2", axResD2);
}

void Analyzers::detail::SensorResidualHists::fill(
    const Storage::TrackState& state, const Storage::Cluster& cluster)
{
  XYVector res = cluster.posLocal() - state.offset();
  SymMatrix2 cov = cluster.covLocal() + state.covOffset();

  resU->Fill(res.x());
  resV->Fill(res.y());
  resDist->Fill(res.r());
  resD2->Fill(mahalanobisSquared(cov, res));
  resUV->Fill(res.x(), res.y());
  trackUResU->Fill(state.offset().x(), res.x());
  trackUResV->Fill(state.offset().x(), res.y());
  trackVResU->Fill(state.offset().y(), res.x());
  trackVResV->Fill(state.offset().y(), res.y());
  slopeUResU->Fill(state.slope().x(), res.x());
  slopeUResV->Fill(state.slope().x(), res.y());
  slopeVResU->Fill(state.slope().y(), res.x());
  slopeVResV->Fill(state.slope().y(), res.y());
}

Analyzers::Residuals::Residuals(TDirectory* dir,
                                const Mechanics::Device& device,
                                const std::vector<Index>& sensorIds,
                                const std::string& subdir,
                                const double rangeStd,
                                const int bins)
{
  for (auto isensor : sensorIds) {
    m_hists_map.emplace(isensor, detail::SensorResidualHists(dir, *device.getSensor(isensor), rangeStd, bins,
                                                             subdir));
  }

}

std::string Analyzers::Residuals::name() const { return "Residuals"; }

void Analyzers::Residuals::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    const Storage::SensorEvent& sev = event.getSensorEvent(isensor);

    if(!m_hists_map.count(sev.sensor()))
      continue;

    auto& hists = m_hists_map[sev.sensor()];

    for (Index icluster = 0; icluster < sev.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = sev.getCluster(icluster);

      if (cluster.isInTrack()) {
        const Storage::TrackState& state = sev.getLocalState(cluster.track());
        hists.fill(state, cluster);
      }
    }
  }
}

void Analyzers::Residuals::finalize() {}


Analyzers::Matching::Matching(TDirectory* dir,
                                const Mechanics::Device& device,
                                const std::vector<Index>& sensorIds,
                                const double rangeStd,
                                const int bins)
{
  for (auto isensor : sensorIds) {
    m_hists_map.emplace(isensor, detail::SensorResidualHists(dir, *device.getSensor(isensor), rangeStd, bins,
                                                             "matching"));
  }


}


std::string Analyzers::Matching::name() const { return "Matching"; }

void Analyzers::Matching::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    const Storage::SensorEvent &sensorEvent = event.getSensorEvent(isensor);

    if (!m_hists_map.count(sensorEvent.sensor()))
      continue;

    auto &hists = m_hists_map[sensorEvent.sensor()];

    // matched pairs
    for (const auto &s : sensorEvent.localStates()) {
      const Storage::TrackState &state = s.second;

      if (state.isMatched()) {
        const Storage::Cluster &cluster = sensorEvent.getCluster(state.matchedCluster());
        hists.fill(state, cluster);
      }
    }
  }
}

void Analyzers::Matching::finalize() {}
