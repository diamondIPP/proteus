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

namespace proteus {

detail::SensorResidualHists::SensorResidualHists(TDirectory* dir,
                                                 const Sensor& sensor,
                                                 double rangeStd,
                                                 int bins,
                                                 const std::string& name)
{
  // always use odd number of bins to have a central bin for zero residual
  bins += ((bins % 2) ? 0 : 1);

  auto box = sensor.sensitiveVolume();
  auto pitch = sensor.pitch();
  Vector2 slopeStdev = extractStdev(sensor.beamSlopeCovariance());
  // ensure sensible histogram limits even for vanishing divergence
  slopeStdev =
      (slopeStdev.array() <= 0).select(Vector2::Constant(1.25e-3), slopeStdev);
  Vector2 slopeMin = sensor.beamSlope() - rangeStd * slopeStdev;
  Vector2 slopeMax = sensor.beamSlope() + rangeStd * slopeStdev;

  auto resUMax = rangeStd * pitch[kU] / std::sqrt(12.0);
  auto resVMax = rangeStd * pitch[kV] / std::sqrt(12.0);
  auto distMax = std::hypot(resUMax, resVMax);
  auto binsU = static_cast<int>(box.length(kU) / pitch[kU]);
  auto binsV = static_cast<int>(box.length(kV) / pitch[kV]);
  auto binsS = static_cast<int>(box.length(kS) / pitch[kS]);

  // residual axes
  HistAxis axResU(-resUMax, resUMax, bins, "Cluster - track position u");
  HistAxis axResV(-resVMax, resVMax, bins, "Cluster - track position v");
  // time is a bit special since it might not be fitted at all but usually
  // has a smaller range. use the full range for the residuals
  auto axResS = HistAxis::Difference(box.interval(kS), pitch[kS],
                                     "Cluster - track local time");
  HistAxis axDist(0, distMax, bins, "Cluster - track distance");
  HistAxis axD2(0, 2 * rangeStd, bins,
                "Cluster - track weighted squared distance");
  // track parameter axes
  HistAxis axU(box.interval(kU), binsU, "Track position u");
  HistAxis axV(box.interval(kV), binsV, "Track position v");
  HistAxis axS(box.interval(kS), binsS, "Track local time");
  HistAxis axSlopeU(slopeMin[0], slopeMax[0], bins, "Track slope u");
  HistAxis axSlopeV(slopeMin[1], slopeMax[1], bins, "Track slope v");

  TDirectory* sub = makeDir(dir, "sensors/" + sensor.name() + "/" + name);
  resU = makeH1(sub, "res_u", axResU);
  posUResU = makeH2(sub, "res_u-position_u", axU, axResU);
  posVResU = makeH2(sub, "res_u-position_v", axV, axResU);
  timeResU = makeH2(sub, "res_u-time", axS, axResU);
  slopeUResU = makeH2(sub, "res_u-slope_u", axSlopeU, axResU);
  slopeVResU = makeH2(sub, "res_u-slope_v", axSlopeV, axResU);
  resV = makeH1(sub, "res_v", axResV);
  posUResV = makeH2(sub, "res_v-position_u", axU, axResV);
  posVResV = makeH2(sub, "res_v-position_v", axV, axResV);
  timeResV = makeH2(sub, "res_v-time", axS, axResV);
  slopeUResV = makeH2(sub, "res_v-slope_u", axSlopeU, axResV);
  slopeVResV = makeH2(sub, "res_v-slope_v", axSlopeV, axResV);
  resUV = makeH2(sub, "res_uv", axResU, axResV);
  resS = makeH1(sub, "res_time", axResS);
  resDist = makeH1(sub, "res_dist", axDist);
  resD2 = makeH1(sub, "res_d2", axD2);
}

void detail::SensorResidualHists::fill(const TrackState& state,
                                       const Cluster& cluster)
{
  Vector4 res = cluster.position() - state.position();
  SymMatrix2 locCov = cluster.uvCov() + state.loc01Cov();

  resU->Fill(res[kU]);
  resV->Fill(res[kV]);
  resS->Fill(res[kS]);
  resUV->Fill(res[kU], res[kV]);
  resDist->Fill(std::hypot(res[kU], res[kV]));
  resD2->Fill(mahalanobisSquared(locCov, res.segment<2>(kU)));
  posUResU->Fill(state.loc0(), res[kU]);
  posUResV->Fill(state.loc0(), res[kV]);
  posVResU->Fill(state.loc1(), res[kU]);
  posVResV->Fill(state.loc1(), res[kV]);
  timeResU->Fill(state.time(), res[kU]);
  timeResV->Fill(state.time(), res[kV]);
  slopeUResU->Fill(state.slopeLoc0(), res[kU]);
  slopeUResV->Fill(state.slopeLoc0(), res[kV]);
  slopeVResU->Fill(state.slopeLoc1(), res[kU]);
  slopeVResV->Fill(state.slopeLoc1(), res[kV]);
}

Residuals::Residuals(TDirectory* dir,
                     const Device& device,
                     const std::vector<Index>& sensorIds,
                     const std::string& subdir,
                     double rangeStd,
                     int bins)
{
  for (auto isensor : sensorIds) {
    m_hists_map.emplace(
        isensor, detail::SensorResidualHists(dir, device.getSensor(isensor),
                                             rangeStd, bins, subdir));
  }
}

std::string Residuals::name() const { return "Residuals"; }

void Residuals::execute(const Event& event)
{
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    const SensorEvent& sev = event.getSensorEvent(isensor);

    if (!m_hists_map.count(isensor)) {
      continue;
    }

    auto& hists = m_hists_map[isensor];

    for (Index icluster = 0; icluster < sev.numClusters(); ++icluster) {
      const Cluster& cluster = sev.getCluster(icluster);

      if (cluster.isInTrack() and sev.hasLocalState(cluster.track())) {
        hists.fill(sev.getLocalState(cluster.track()), cluster);
      }
    }
  }
}

Matching::Matching(TDirectory* dir,
                   const Sensor& sensor,
                   const double rangeStd,
                   const int bins)
    : m_sensorId(sensor.id()), m_hists(dir, sensor, rangeStd, bins, "matching")
{
}

std::string Matching::name() const { return "Matching"; }

void Matching::execute(const Event& event)
{
  const SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  // iterate over all matched pairs
  for (const auto& state : sensorEvent.localStates()) {
    if (state.isMatched()) {
      const Cluster& cluster = sensorEvent.getCluster(state.matchedCluster());
      m_hists.fill(state, cluster);
    }
  }
}

} // namespace proteus
