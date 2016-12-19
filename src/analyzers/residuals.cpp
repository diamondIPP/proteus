#include "residuals.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
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
  auto rangeU = sensor.sensitiveAreaLocal().axes[0];
  auto rangeV = sensor.sensitiveAreaLocal().axes[1];
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
  trackVResU = makeH2(dir, name("ResU_TrackV"), axTrackV, axResV);
  slopeUResU = makeH2(dir, name("ResU_SlopeU"), axSlopeU, axResU);
  slopeVResU = makeH2(dir, name("ResU_SlopeV"), axSlopeV, axResV);
  resV = makeH1(dir, name("ResV"), axResV);
  trackUResV = makeH2(dir, name("ResV_TrackU"), axTrackV, axResU);
  trackVResV = makeH2(dir, name("ResV_TrackV"), axTrackV, axResV);
  slopeUResV = makeH2(dir, name("ResV_SlopeU"), axSlopeU, axResU);
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
                                const char* suffix,
                                const double pixelRange,
                                const double slopeRange,
                                const int bins)
    : SingleAnalyzer(device, dir, suffix, "Residuals")
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = makeGetDirectory("Residuals");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    m_hists.emplace_back(sub, *device->getSensor(isensor), pixelRange,
                         slopeRange, bins);
  }
}

void Analyzers::Residuals::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (Index itrack = 0; itrack < event->numTracks(); itrack++) {
    const Storage::Track& track = *event->getTrack(itrack);

    // Check if the track passes the cuts
    if (!checkCuts(&track))
      continue;

    for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *track.getCluster(icluster);
      const Mechanics::Sensor& sensor = *_device->getSensor(cluster.sensorId());
      Storage::TrackState state = Processors::fitTrackLocal(track, sensor);

      m_hists[cluster.sensorId()].fill(state, cluster);
    }
  }
}

void Analyzers::Residuals::postProcessing() {} // Needs to be declared even if
                                               // not used

TH1D* Analyzers::Residuals::getResidualX(Index sensorId)
{
  validSensor(sensorId);
  return m_hists.at(sensorId).resU;
}

TH1D* Analyzers::Residuals::getResidualY(Index sensorId)
{
  validSensor(sensorId);
  return m_hists.at(sensorId).resV;
}

TH2D* Analyzers::Residuals::getResidualXX(Index sensorId)
{
  validSensor(sensorId);
  return m_hists.at(sensorId).trackUResU;
}

TH2D* Analyzers::Residuals::getResidualXY(Index sensorId)
{
  validSensor(sensorId);
  return m_hists.at(sensorId).trackUResV;
}

TH2D* Analyzers::Residuals::getResidualYY(Index sensorId)
{
  validSensor(sensorId);
  return m_hists.at(sensorId).trackVResV;
}

TH2D* Analyzers::Residuals::getResidualYX(Index sensorId)
{
  validSensor(sensorId);
  return m_hists.at(sensorId).trackVResU;
}

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
      const Mechanics::Sensor& sensor = *m_device.getSensor(cluster.sensorId());
      // refit w/o current sensor information
      Storage::TrackState state =
          Processors::fitTrackLocalUnbiased(track, sensor);

      m_hists[cluster.sensorId()].fill(state, cluster);
    }
  }
}

void Analyzers::UnbiasedResiduals::finalize() {}

const TH2D* Analyzers::UnbiasedResiduals::getResidualUV(Index sensorId) const
{
  return m_hists.at(sensorId).resUV;
}
