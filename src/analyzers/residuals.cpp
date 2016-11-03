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

    double maxDist = 10 * std::max(sensor.pitchCol(), sensor.pitchRow());
    double maxSlope = 0.001;
    size_t nBins = 10 * 16;
    Sensor::Area area = sensor.sensitiveAreaLocal();
    Sensor::Area::Axis dist = {-maxDist, maxDist};
    Sensor::Area::Axis slope = {-maxSlope, maxSlope};

    auto makeH2 = [&](const char* suffix, const Sensor::Area::Axis& bounds0,
                      const Sensor::Area::Axis& bounds1) {
      TH2D* h =
          new TH2D((sensor.name() + suffix).c_str(), "", nBins, bounds0.min,
                   bounds0.max, nBins, bounds1.min, bounds1.max);
      h->SetDirectory(dir);
      return h;
    };
    m_res.push_back(makeH2("-res", dist, dist));
    m_resUtrackU.push_back(makeH2("-resUtrackU", area.axes[0], dist));
    m_resUtrackV.push_back(makeH2("-resUtrackV", area.axes[0], dist));
    m_resVtrackU.push_back(makeH2("-resVtrackU", area.axes[1], dist));
    m_resVtrackV.push_back(makeH2("-resVtrackV", area.axes[1], dist));
    m_resUslopeU.push_back(makeH2("-resUslopeU", slope, dist));
    m_resUslopeV.push_back(makeH2("-resUslopeV", slope, dist));
    m_resVslopeU.push_back(makeH2("-resVslopeU", slope, dist));
    m_resVslopeV.push_back(makeH2("-resVslopeV", slope, dist));
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
      Index sensorId = cluster.sensorId();

      // refit track w/o selected sensor
      Storage::TrackState state =
          Processors::fitTrackLocalUnbiased(track, sensor, sensorId);
      XYPoint clu = sensor.transformPixelToLocal(cluster.posPixel());
      XYVector res = clu - state.offset();

      m_res[sensorId]->Fill(res.x(), res.y());
      m_resUtrackU[sensorId]->Fill(state.offset().x(), res.x());
      m_resUtrackV[sensorId]->Fill(state.offset().y(), res.x());
      m_resVtrackU[sensorId]->Fill(state.offset().x(), res.y());
      m_resVtrackV[sensorId]->Fill(state.offset().y(), res.y());
      m_resUslopeU[sensorId]->Fill(state.slope().x(), res.x());
      m_resUslopeV[sensorId]->Fill(state.slope().y(), res.x());
      m_resVslopeU[sensorId]->Fill(state.slope().x(), res.y());
      m_resVslopeV[sensorId]->Fill(state.slope().y(), res.y());
    }
  }
}

void Analyzers::UnbiasedResiduals::finalize() {}
