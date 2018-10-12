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
  auto distRange = std::sqrt(resMaxU * resMaxU + resMaxV * resMaxV);
  // maximum time differences computable, max value is exclusive
  auto timedeltaMin = sensor.timeRange().min() - sensor.timeRange().max() + 1;
  auto timedeltaMax = sensor.timeRange().max() - sensor.timeRange().min();

  Vector2 slopeMin = sensor.beamSlope() - rangeStd * sensor.beamDivergence();
  Vector2 slopeMax = sensor.beamSlope() + rangeStd * sensor.beamDivergence();

  HistAxis axResU(-resMaxU, resMaxU, bins, "Cluster - track residual u");
  HistAxis axResV(-resMaxV, resMaxV, bins, "Cluster - track residual v");
  HistAxis axPosU(posRangeU, bins, "Local track position u");
  HistAxis axPosV(posRangeV, bins, "Local track position v");
  HistAxis axSlopeU(slopeMin[0], slopeMax[0], bins, "Local track slope u");
  HistAxis axSlopeV(slopeMin[1], slopeMax[1], bins, "Local track slope v");
  HistAxis axResDist(0, distRange, bins, "Cluster - track distance");
  HistAxis axResD2(0, 2 * rangeStd, bins,
                   "Cluster - track weighted squared distance");
  HistAxis axResTime(timedeltaMin - 0.5, timedeltaMax - 0.5,
                     timedeltaMax - timedeltaMin, "Cluster - track time");

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
  resTime = makeH1(sub, "res_time", axResTime);
}

void Analyzers::detail::SensorResidualHists::fill(
    const Storage::TrackState& state, const Storage::Cluster& cluster)
{
  Vector2 res = cluster.posLocal() - state.offset();
  SymMatrix2 cov = cluster.covLocal() + state.covOffset();

  resU->Fill(res[0]);
  resV->Fill(res[1]);
  resDist->Fill(std::hypot(res[0], res[1]));
  resD2->Fill(mahalanobisSquared(cov, res));
  resTime->Fill(cluster.time() - state.time());
  resUV->Fill(res[0], res[1]);
  trackUResU->Fill(state.offset()[0], res[0]);
  trackUResV->Fill(state.offset()[0], res[1]);
  trackVResU->Fill(state.offset()[1], res[0]);
  trackVResV->Fill(state.offset()[1], res[1]);
  slopeUResU->Fill(state.slope()[0], res[0]);
  slopeUResV->Fill(state.slope()[0], res[1]);
  slopeVResU->Fill(state.slope()[1], res[0]);
  slopeVResV->Fill(state.slope()[1], res[1]);
}

Analyzers::Residuals::Residuals(TDirectory* dir,
                                const Mechanics::Device& device,
                                const std::vector<Index>& sensorIds,
                                const std::string& subdir,
                                const double rangeStd,
                                const int bins)
{
  for (auto isensor : sensorIds) {
    m_hists_map.emplace(
        isensor, detail::SensorResidualHists(dir, device.getSensor(isensor),
                                             rangeStd, bins, subdir));
  }
}

std::string Analyzers::Residuals::name() const { return "Residuals"; }

void Analyzers::Residuals::execute(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    const Storage::SensorEvent& sev = event.getSensorEvent(isensor);

    if (!m_hists_map.count(sev.sensor()))
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

Analyzers::Matching::Matching(TDirectory* dir,
                              const Mechanics::Sensor& sensor,
                              const double rangeStd,
                              const int bins)
    : m_hists(dir, sensor, rangeStd, bins, "matching")
{
}

std::string Analyzers::Matching::name() const { return "Matching"; }

void Analyzers::Matching::execute(const Storage::Event& event)
{
  const Storage::SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  // iterate over all matched pairs
  for (const auto& s : sensorEvent.localStates()) {
    const Storage::TrackState& state = s.second;
    if (state.isMatched()) {
      const Storage::Cluster& cluster =
          sensorEvent.getCluster(state.matchedCluster());
      m_hists.fill(state, cluster);
    }
  }
}
