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
    const double pixelRange,
    const double slopeRange,
    const int bins,
    const std::string& name)
{
  using namespace Utils;

  double resRange =
      pixelRange * std::hypot(sensor.pitchCol(), sensor.pitchRow());
  auto rangeU = sensor.sensitiveAreaLocal().interval(0);
  auto rangeV = sensor.sensitiveAreaLocal().interval(1);

  TDirectory* sub = makeDir(dir, sensor.name() + "/" + name);

  HistAxis axResU(-resRange, resRange, bins, "Cluster - track residual u");
  HistAxis axResV(-resRange, resRange, bins, "Cluster - track residual v");
  HistAxis axPosU(rangeU, bins, "Local track position u");
  HistAxis axPosV(rangeV, bins, "Local track position v");
  HistAxis axSlopeU(-slopeRange, slopeRange, bins, "Local track slope u");
  HistAxis axSlopeV(-slopeRange, slopeRange, bins, "Local track slope v");

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
}

void Analyzers::detail::SensorResidualHists::fill(
    const Storage::TrackState& state, const Storage::Cluster& cluster)
{
  XYVector res = cluster.posLocal() - state.offset();
  resU->Fill(res.x());
  resV->Fill(res.y());
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
                                const std::string& subdir,
                                const double pixelRange,
                                const double slopeRange,
                                const int bins)
{
  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    m_hists.emplace_back(dir, *device.getSensor(isensor), pixelRange,
                         slopeRange, bins, subdir);
  }
}

std::string Analyzers::Residuals::name() const { return "Residuals"; }

void Analyzers::Residuals::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    const Storage::SensorEvent& sev = event.getSensorEvent(isensor);
    auto& hists = m_hists[isensor];

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
