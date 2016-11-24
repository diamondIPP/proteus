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

namespace Analyzers {

void Residuals::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (Index itrack = 0; itrack < event->numTracks(); itrack++) {
    const Storage::Track* track = event->getTrack(itrack);

    // Check if the track passes the cuts
    if (!checkCuts(track))
      continue;

    for (Index icluster = 0; icluster < track->numClusters(); ++icluster) {
      const Storage::Cluster* cluster = track->getCluster(icluster);
      Index sensorId = cluster->sensorId();
      const Mechanics::Sensor* sensor = _device->getSensor(sensorId);

      double tx = 0, ty = 0, tz = 0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      XYZPoint t(tx, ty, tz);
      XYZVector d = cluster->posGlobal() - t;

      _residualsX.at(sensorId)->Fill(d.x());
      _residualsY.at(sensorId)->Fill(d.y());
      _residualsXX.at(sensorId)->Fill(d.x(), t.x());
      _residualsYY.at(sensorId)->Fill(d.y(), t.y());
      _residualsXY.at(sensorId)->Fill(d.x(), t.y());
      _residualsYX.at(sensorId)->Fill(d.y(), t.x());
    }
  }
}

void Residuals::postProcessing() {} // Needs to be declared even if not used

TH1D* Residuals::getResidualX(unsigned int nsensor)
{
  validSensor(nsensor);
  return _residualsX.at(nsensor);
}

TH1D* Residuals::getResidualY(unsigned int nsensor)
{
  validSensor(nsensor);
  return _residualsY.at(nsensor);
}

TH2D* Residuals::getResidualXX(unsigned int nsensor)
{
  validSensor(nsensor);
  return _residualsXX.at(nsensor);
}

TH2D* Residuals::getResidualXY(unsigned int nsensor)
{
  validSensor(nsensor);
  return _residualsXY.at(nsensor);
}

TH2D* Residuals::getResidualYY(unsigned int nsensor)
{
  validSensor(nsensor);
  return _residualsYY.at(nsensor);
}

TH2D* Residuals::getResidualYX(unsigned int nsensor)
{
  validSensor(nsensor);
  return _residualsYX.at(nsensor);
}

Residuals::Residuals(const Mechanics::Device* refDevice,
                     TDirectory* dir,
                     const char* suffix,
                     unsigned int nPixX,
                     double binsPerPix,
                     unsigned int binsY)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(refDevice, dir, suffix, "Residuals")
{
  assert(refDevice && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* dir1d = makeGetDirectory("Residuals1D");
  TDirectory* dir2d = makeGetDirectory("Residuals2D");

  std::stringstream name;  // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo

  std::stringstream xAxisTitle;
  std::stringstream yAxisTitle;

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++) {
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);
    for (unsigned int axis = 0; axis < 2; axis++) {
      double width =
          nPixX * (axis ? sensor->getPosPitchX() : sensor->getPosPitchY());

      unsigned int nbins = binsPerPix * nPixX;
      if (!(nbins % 2))
        nbins += 1;
      double height = 0;

      // Generate the 1D residual distribution for the given axis
      name.str("");
      title.str("");
      name << sensor->getName() << ((axis) ? "X" : "Y") << _nameSuffix;
      title << sensor->getName() << ((axis) ? " X" : " Y")
            << ";Track cluster difference " << (axis ? "X" : "Y") << " ["
            << _device->getSpaceUnit() << "]"
            << ";Clusters / " << 1.0 / binsPerPix << " pixel";
      TH1D* res1d = new TH1D(name.str().c_str(), title.str().c_str(), nbins,
                             -width / 2.0, width / 2.0);
      res1d->SetDirectory(dir1d);
      if (axis)
        _residualsX.push_back(res1d);
      else
        _residualsY.push_back(res1d);

      // Generate the XX or YY residual distribution

      // The height of this plot depends on the sensor and X or Y axis
      height = axis ? sensor->getPosSensitiveX() : sensor->getPosSensitiveY();

      name.str("");
      title.str("");
      name << sensor->getName() << ((axis) ? "XvsX" : "YvsY") << _nameSuffix;
      title << sensor->getName() << ((axis) ? " X vs. X" : " Y vs. Y")
            << ";Track cluster difference " << (axis ? "X" : "Y") << " ["
            << _device->getSpaceUnit() << "]"
            << ";Track position " << (axis ? "X" : "Y") << " ["
            << _device->getSpaceUnit() << "]"
            << ";Clusters / " << 1.0 / binsPerPix << " pixel";
      TH2D* resAA =
          new TH2D(name.str().c_str(), title.str().c_str(), nbins, -width / 2.0,
                   width / 2.0, binsY, -height / 2.0, height / 2.0);
      resAA->SetDirectory(dir2d);
      if (axis)
        _residualsXX.push_back(resAA);
      else
        _residualsYY.push_back(resAA);

      // Generate the XY or YX distribution

      height = axis ? sensor->getSensitiveY() : sensor->getSensitiveX();

      name.str("");
      title.str("");
      name << sensor->getName() << ((axis) ? "XvsY" : "YvsX") << _nameSuffix;
      title << sensor->getName() << ((axis) ? " X vs. Y" : " Y vs. X")
            << ";Track cluster difference " << (axis ? "X" : "Y") << " ["
            << _device->getSpaceUnit() << "]"
            << ";Track position " << (axis ? "Y" : "X") << " ["
            << _device->getSpaceUnit() << "]"
            << ";Clusters / " << 1.0 / binsPerPix << " pixel";
      TH2D* resAB =
          new TH2D(name.str().c_str(), title.str().c_str(), nbins, -width / 2.0,
                   width / 2.0, binsY, -height / 2.0, height / 2.0);
      resAB->SetDirectory(dir2d);
      if (axis)
        _residualsXY.push_back(resAB);
      else
        _residualsYX.push_back(resAB);
    }
  }
}
}

Analyzers::UnbiasedResiduals::UnbiasedResiduals(const Mechanics::Device& device,
                                                TDirectory* parent)
    : m_device(device)
{
  using Mechanics::Sensor;

  TDirectory* dir = parent->mkdir("UnbiasedResiduals");

  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    const Sensor& sensor = *device.getSensor(isensor);

    int pixels = 3;
    int nBinsPixel = 20;
    int nBinsDist = 2 * pixels * nBinsPixel;
    int nBinsX = 100;
    double maxSlope = 0.001;
    Sensor::Area::Axis rangeU = sensor.sensitiveAreaLocal().axes[0];
    Sensor::Area::Axis rangeV = sensor.sensitiveAreaLocal().axes[1];
    Sensor::Area::Axis slope = {-maxSlope, maxSlope};
    Sensor::Area::Axis distU = {-pixels * sensor.pitchCol(),
                                pixels * sensor.pitchCol()};
    Sensor::Area::Axis distV = {-pixels * sensor.pitchRow(),
                                pixels * sensor.pitchRow()};

    auto makeH2 = [&](const char* suffix, size_t nbins0,
                      const Sensor::Area::Axis& bounds0, size_t nbins1,
                      const Sensor::Area::Axis& bounds1) {
      TH2D* h =
          new TH2D((sensor.name() + suffix).c_str(), "", nbins0, bounds0.min,
                   bounds0.max, nbins1, bounds1.min, bounds1.max);
      h->SetDirectory(dir);
      return h;
    };
    Hists hists;
    hists.res = makeH2("-Res", nBinsDist, distU, nBinsDist, distV);
    hists.trackUResU = makeH2("-TrackUResU", nBinsX, rangeU, nBinsDist, distU);
    hists.trackUResV = makeH2("-TrackUResV", nBinsX, rangeU, nBinsDist, distV);
    hists.trackVResU = makeH2("-TrackVResU", nBinsX, rangeV, nBinsDist, distU);
    hists.trackVResV = makeH2("-TrackVResV", nBinsX, rangeV, nBinsDist, distV);
    hists.slopeUResU = makeH2("-SlopeUResU", nBinsX, slope, nBinsDist, distU);
    hists.slopeUResV = makeH2("-SlopeUResV", nBinsX, slope, nBinsDist, distV);
    hists.slopeVResU = makeH2("-SlopeVResU", nBinsX, slope, nBinsDist, distU);
    hists.slopeVResV = makeH2("-SlopeVResV", nBinsX, slope, nBinsDist, distV);
    m_hists.push_back(hists);
  }
}

std::string Analyzers::UnbiasedResiduals::UnbiasedResiduals::name() const
{
  return "unbiased_residuals";
}

void Analyzers::UnbiasedResiduals::analyze(const Storage::Event& event)
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);
    for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *track.getCluster(icluster);
      const Mechanics::Sensor& sensor = *m_device.getSensor(cluster.sensorId());
      Index sensorId = cluster.sensorId();

      // refit track w/o selected sensor
      Storage::TrackState state =
          Processors::fitTrackLocalUnbiased(track, sensor);
      XYPoint clu = sensor.transformPixelToLocal(cluster.posPixel());
      XYVector res = clu - state.offset();

      Hists& hists = m_hists[sensorId];
      hists.res->Fill(res.x(), res.y());
      hists.trackUResU->Fill(state.offset().x(), res.x());
      hists.trackUResV->Fill(state.offset().x(), res.y());
      hists.trackVResU->Fill(state.offset().y(), res.x());
      hists.trackVResV->Fill(state.offset().y(), res.y());
      hists.slopeUResU->Fill(state.slope().x(), res.x());
      hists.slopeUResV->Fill(state.slope().x(), res.y());
      hists.slopeVResU->Fill(state.slope().y(), res.x());
      hists.slopeVResV->Fill(state.slope().y(), res.y());
    }
  }
}

void Analyzers::UnbiasedResiduals::finalize() {}

const TH2D* Analyzers::UnbiasedResiduals::getResidualUV(Index sensorId) const
{
  return m_hists.at(sensorId).res;
}
