#include "residuals.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_GLOBAL_LOGGER

Analyzers::detail::SensorResidualHists::SensorResidualHists(
    TDirectory* dir,
    const Mechanics::Sensor& sensor,
    const double pixelRange,
    const double slopeRange,
    const int bins)
{
  using namespace Utils;

  double resRange =
      pixelRange * std::hypot(sensor.pitchCol(), sensor.pitchRow());
  auto rangeU = sensor.sensitiveAreaLocal().interval(0);
  auto rangeV = sensor.sensitiveAreaLocal().interval(1);
  auto name = [&](const std::string& suffix) {
    return sensor.name() + '-' + suffix;
  };

  HistAxis axResU(-resRange, resRange, bins, "Cluster - track residual u");
  HistAxis axResV(-resRange, resRange, bins, "Cluster - track residual v");
  HistAxis axTrackU(rangeU, bins, "Local track position u");
  HistAxis axTrackV(rangeV, bins, "Local track position v");
  HistAxis axSlopeU(-slopeRange, slopeRange, bins, "Local track slope u");
  HistAxis axSlopeV(-slopeRange, slopeRange, bins, "Local track slope v");

  resU = makeH1(dir, name("ResU"), axResU);
  trackUResU = makeH2(dir, name("ResU_TrackU"), axTrackU, axResU);
  trackVResU = makeH2(dir, name("ResU_TrackV"), axTrackV, axResU);
  slopeUResU = makeH2(dir, name("ResU_SlopeU"), axSlopeU, axResU);
  slopeVResU = makeH2(dir, name("ResU_SlopeV"), axSlopeV, axResU);
  resV = makeH1(dir, name("ResV"), axResV);
  trackUResV = makeH2(dir, name("ResV_TrackU"), axTrackU, axResV);
  trackVResV = makeH2(dir, name("ResV_TrackV"), axTrackV, axResV);
  slopeUResV = makeH2(dir, name("ResV_SlopeU"), axSlopeU, axResV);
  slopeVResV = makeH2(dir, name("ResV_SlopeV"), axSlopeV, axResV);
  resUV = makeH2(dir, name("ResUV"), axResU, axResV);
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

Analyzers::Residuals::Residuals(const Mechanics::Device* device,
                                TDirectory* dir,
                                const double pixelRange,
                                const double slopeRange,
                                const int bins)
    : m_device(*device)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = Utils::makeDir(dir, "Residuals");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    m_hists.emplace_back(sub, *device->getSensor(isensor), pixelRange,
                         slopeRange, bins);
  }
}

std::string Analyzers::Residuals::name() const { return "Residuals"; }

void Analyzers::Residuals::analyze(const Storage::Event& event)
{
  for (Index itrack = 0; itrack < event.numTracks(); itrack++) {
    const Storage::Track& track = *event.getTrack(itrack);

    for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *track.getCluster(icluster);
      Storage::TrackState state = Processors::fitTrackLocal(
          track, m_device.geometry(), cluster.sensorId());

      m_hists[cluster.sensorId()].fill(state, cluster);
    }
  }
}

void Analyzers::Residuals::finalize() {}

Analyzers::UnbiasedResiduals::UnbiasedResiduals(const Mechanics::Device& device,
                                                TDirectory* parent,
                                                const double pixelRange,
                                                const double slopeRange,
                                                const int bins)
    : m_device(device)
{
  using Mechanics::Sensor;

  TDirectory* dir = parent->mkdir("UnbiasedResiduals");

  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    m_hists.emplace_back(dir, *device.getSensor(isensor), pixelRange,
                         slopeRange, bins);
  }
}

std::string Analyzers::UnbiasedResiduals::UnbiasedResiduals::name() const
{
  return "UnbiasedResiduals";
}

void Analyzers::UnbiasedResiduals::analyze(const Storage::Event& event)
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);
    for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *track.getCluster(icluster);
      // refit w/o current sensor information
      Storage::TrackState state = Processors::fitTrackLocalUnbiased(
          track, m_device.geometry(), cluster.sensorId());

      m_hists[cluster.sensorId()].fill(state, cluster);
    }
  }
}

void Analyzers::UnbiasedResiduals::finalize() {}

const TH2D* Analyzers::UnbiasedResiduals::getResidualUV(Index sensorId) const
{
  return m_hists.at(sensorId).resUV;
}
