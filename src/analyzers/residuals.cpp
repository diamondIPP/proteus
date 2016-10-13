#include "residuals.h"

#include <cassert>
#include <math.h>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

// Access to the device being analyzed and its sensors
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
// Access to the data stored in the event
#include "../storage/cluster.h"
#include "../storage/event.h"
#include "../storage/hit.h"
#include "../storage/plane.h"
#include "../storage/track.h"
// Some generic processors to calcualte typical event related things
#include "../processors/processors.h"

namespace Analyzers {

void Residuals::processEvent(const Storage::Event* refEvent)
{
  assert(refEvent && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(refEvent);

  // Check if the event passes the cuts
  if (!checkCuts(refEvent))
    return;

  for (unsigned int ntrack = 0; ntrack < refEvent->numTracks(); ntrack++) {
    const Storage::Track* track = refEvent->getTrack(ntrack);

    // Check if the track passes the cuts
    if (!checkCuts(track))
      continue;

    for (unsigned int nplane = 0; nplane < refEvent->numPlanes(); nplane++) {
      const Storage::Plane* plane = refEvent->getPlane(nplane);
      const Mechanics::Sensor* sensor = _device->getSensor(nplane);
      double tx = 0, ty = 0, tz = 0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      // fdibello@cern.ch variable to select the closest cluster associated to
      // the track
      double rx1 = 0, ry1 = 0, dist = 0, dist1 = 1000000;

      for (unsigned int ncluster = 0; ncluster < plane->numClusters();
           ncluster++) {
        const Storage::Cluster* cluster = plane->getCluster(ncluster);

        // Check if the cluster passes the cuts
        if (!checkCuts(cluster))
          continue;

        const double rx = tx - cluster->getPosX();
        const double ry = ty - cluster->getPosY();
        dist = sqrt(pow(rx / sensor->getPitchX(), 2) +
                    pow(ry / sensor->getPitchY(), 2));
        // fdibello@cern.ch only the closest cluster w.r.t. the track is taken
        if (dist < dist1) {
          rx1 = rx;
          ry1 = ry;
          dist1 = dist;
        }
      }

      _residualsX.at(nplane)->Fill(rx1);
      _residualsY.at(nplane)->Fill(ry1);
      _residualsXX.at(nplane)->Fill(rx1, tx);
      _residualsYY.at(nplane)->Fill(ry1, ty);
      _residualsXY.at(nplane)->Fill(rx1, ty);
      _residualsYX.at(nplane)->Fill(ry1, tx);
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
      TH1D* res1d = new TH1D(name.str().c_str(),
                             title.str().c_str(),
                             nbins,
                             -width / 2.0,
                             width / 2.0);
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
      TH2D* resAA = new TH2D(name.str().c_str(),
                             title.str().c_str(),
                             nbins,
                             -width / 2.0,
                             width / 2.0,
                             binsY,
                             -height / 2.0,
                             height / 2.0);
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
      TH2D* resAB = new TH2D(name.str().c_str(),
                             title.str().c_str(),
                             nbins,
                             -width / 2.0,
                             width / 2.0,
                             binsY,
                             -height / 2.0,
                             height / 2.0);
      resAB->SetDirectory(dir2d);
      if (axis)
        _residualsXY.push_back(resAB);
      else
        _residualsYX.push_back(resAB);
    }
  }
}
}
