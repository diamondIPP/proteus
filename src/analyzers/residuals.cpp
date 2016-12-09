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

PT_SETUP_GLOBAL_LOGGER

Analyzers::detail::SensorResidualHists::SensorResidualHists(
    TDirectory* dir, const Mechanics::Sensor& sensor, double slopeMax)
{
  typedef Mechanics::Sensor::Area::Axis Interval;

  const int pixels = 3;
  const int nBinsPixel = 20;
  const int nBinsDist = 2 * pixels * nBinsPixel;
  const int nBinsX = 100;
  Interval rangeU = sensor.sensitiveAreaLocal().axes[0];
  Interval rangeV = sensor.sensitiveAreaLocal().axes[1];
  Interval rangeSlope = {-slopeMax, slopeMax};
  Interval distU = {-pixels * sensor.pitchCol(), pixels * sensor.pitchCol()};
  Interval distV = {-pixels * sensor.pitchRow(), pixels * sensor.pitchRow()};

  auto h1 = [&](const char* name, size_t nx, const Interval& x) {
    TH1D* h =
        new TH1D((sensor.name() + '-' + name).c_str(), "", nx, x.min, x.max);
    h->SetDirectory(dir);
    return h;
  };
  auto h2 = [&](const char* name, size_t nx, const Interval& x, size_t ny,
                const Interval& y) {
    TH2D* h = new TH2D((sensor.name() + '-' + name).c_str(), "", nx, x.min,
                       x.max, ny, y.min, y.max);
    h->SetDirectory(dir);
    return h;
  };
  resU = h1("ResU", nBinsDist, distU);
  trackUResU = h2("ResU_TrackU", nBinsX, rangeU, nBinsDist, distU);
  trackVResU = h2("ResU_TrackV", nBinsX, rangeV, nBinsDist, distU);
  slopeUResU = h2("ResU_SlopeU", nBinsX, rangeSlope, nBinsDist, distU);
  slopeVResU = h2("ResU_SlopeV", nBinsX, rangeSlope, nBinsDist, distU);
  resV = h1("ResV", nBinsDist, distV);
  trackUResV = h2("ResV_TrackU", nBinsX, rangeU, nBinsDist, distV);
  trackVResV = h2("ResV_TrackV", nBinsX, rangeV, nBinsDist, distV);
  slopeUResV = h2("ResV_SlopeU", nBinsX, rangeSlope, nBinsDist, distV);
  slopeVResV = h2("ResV_SlopeV", nBinsX, rangeSlope, nBinsDist, distV);
  resUV = h2("ResUV", nBinsDist, distU, nBinsDist, distV);
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
                                unsigned int nPixX,
                                double binsPerPix,
                                unsigned int binsY)
    : SingleAnalyzer(device, dir, suffix, "Residuals")
{
  assert(refDevice && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = makeGetDirectory("Residuals");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    m_hists.emplace_back(sub, *device->getSensor(isensor));
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
                                                TDirectory* parent)
    : m_device(device)
{
  using Mechanics::Sensor;

  TDirectory* dir = parent->mkdir("UnbiasedResiduals");

  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    m_hists.emplace_back(dir, *device.getSensor(isensor));
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
