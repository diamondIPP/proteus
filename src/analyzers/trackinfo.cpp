#include "trackinfo.h"

#include <cassert>
#include <sstream>
#include <math.h>

#include <TDirectory.h>
#include <TH2D.h>
#include <TH1D.h>

// Access to the device being analyzed and its sensors
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
// Access to the data stored in the event
#include "../storage/hit.h"
#include "../storage/cluster.h"
#include "../storage/plane.h"
#include "../storage/track.h"
#include "../storage/event.h"
// Some generic processors to calcualte typical event related things
#include "../processors/processors.h"
// This header defines all the cuts
#include "cuts.h"

namespace Analyzers {

void TrackInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (unsigned int ntrack = 0; ntrack < event->getNumTracks(); ntrack++)
  {
    const Storage::Track* track = event->getTrack(ntrack);

    // Check if the track passes the cuts
    if (!checkCuts(track))
      continue;

    _slopesX->Fill(track->getSlopeX());
    _slopesY->Fill(track->getSlopeY());
    _origins->Fill(track->getOriginX(), track->getOriginY());
    _originsX->Fill(track->getOriginX());
    _originsY->Fill(track->getOriginY());
    _chi2->Fill(track->getChi2());
    _numClusters->Fill(track->getNumClusters());

    for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++)
    {
      const Mechanics::Sensor* sensor = _device->getSensor(nsens);

      double tx = 0, ty = 0, tz = 0;
      
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);
      //else{tz = 451000;}

      double errX = 0, errY = 0;
      Processors::trackError(track, tz, errX, errY);

      _resX.at(nsens)->Fill(errX);
      _resY.at(nsens)->Fill(errY);
    }

    // bilbao@cern.ch: just filling histograms for X and Y at the DUT position to evaluate the resolution of the telescope.
    double resoX = 0, resoY = 0, tz = 320000;
    Processors::trackError(track, tz, resoX, resoY);
    _resoX->Fill(resoX);
    _resoY->Fill(resoY);

  }
}

void TrackInfo::postProcessing() { }

TrackInfo::TrackInfo(const Mechanics::Device* device,
                     TDirectory* dir,
                     const char* suffix,
                     double resWidth,
                     double maxSlope,
                     double increaseArea) :
  // Base class is initialized here and manages directory / device
  SingleAnalyzer(device, dir, suffix)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("TrackInfo");

  std::stringstream name; // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo  std::stringstream xAxisTitle;

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++)
  {
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);
    for (unsigned int axis = 0; axis < 2; axis++)
    {
      const double width = resWidth *
          (axis ? sensor->getPosPitchX() : sensor->getPosPitchY());

      name.str(""); title.str("");
      name << sensor->getName()
           <<  "Resolution" << (axis ? "X" : "Y") << _nameSuffix;
      title << sensor->getName()
            << " Track Resolution" << (axis ? " X" : " Y")
            << ";Track resolution [" << _device->getSpaceUnit() << "]"
            << ";Tracks";
      TH1D* res = new TH1D(name.str().c_str(), title.str().c_str(),
                           30, 0, width);
      res->SetDirectory(plotDir);
      if (axis) _resX.push_back(res);
      else      _resY.push_back(res);
    }
  }

  name.str(""); title.str("");
  name << "ResolutionAtDUTX";
  title << "ResolutionAtDUTX"
        << ";resolution [um]"
        << ";Tracks";
  _resoX = new TH1D(name.str().c_str(), title.str().c_str(),
		    100, 0, 100);
  _resoX->SetDirectory(plotDir);

  name.str(""); title.str("");
  name << "ResolutionAtDUTY";
  title << "ResolutionAtDUTY"
        << ";resolution [um]"
        << ";Tracks";
  _resoY = new TH1D(name.str().c_str(), title.str().c_str(),
		    100, 0, 100);
  _resoY->SetDirectory(plotDir);


  // Use DUT 0 as a reference sensor for plots axis
  assert(_device->getNumSensors() && "TrackInfo: expects device to have sensors");
  const Mechanics::Sensor* sensor = _device->getSensor(0);

  const double deviceLength =
      device->getSensor(device->getNumSensors() - 1)->getOffZ() -
      device->getSensor(0)->getOffZ();
  const double maxSlopeX = maxSlope * sensor->getPosPitchX() / deviceLength;
  const double maxSlopeY = maxSlope * sensor->getPosPitchY() / deviceLength;

  name.str(""); title.str("");
  name << _device->getName() << "SlopesX" << _nameSuffix;
  title << _device->getName() << " Slopes X"
        << ";Slope [radians]"
        << ";Tracks";
  _slopesX = new TH1D(name.str().c_str(), title.str().c_str(),
                      100, -maxSlopeX, maxSlopeX);
  _slopesX->SetDirectory(plotDir);

  name.str(""); title.str("");
  name << _device->getName() << "SlopesY" << _nameSuffix;
  title << _device->getName() << " Slopes Y"
        << ";Slope [radians]"
        << ";Tracks";
  _slopesY = new TH1D(name.str().c_str(), title.str().c_str(),
                      100, -maxSlopeY, maxSlopeY);
  _slopesY->SetDirectory(plotDir);

  const unsigned int nx = increaseArea * sensor->getPosNumX();
  const unsigned int ny = increaseArea * sensor->getPosNumY();
  const double lowX = sensor->getOffX() - increaseArea * sensor->getPosSensitiveX() / 2.0;
  const double uppX = sensor->getOffX() + increaseArea * sensor->getPosSensitiveX() / 2.0;
  const double lowY = sensor->getOffY() - increaseArea * sensor->getPosSensitiveY() / 2.0;
  const double uppY = sensor->getOffY() + increaseArea * sensor->getPosSensitiveY() / 2.0;

  name.str(""); title.str("");
  name << _device->getName() << "Origins" << _nameSuffix;
  title << _device->getName() << " Origins"
        << ";Origin position X [" << _device->getSpaceUnit() << "]"
        << ";Origin position Y [" << _device->getSpaceUnit() << "]"
        << ";Tracks";
  _origins = new TH2D(name.str().c_str(), title.str().c_str(),
                      nx, lowX, uppX,
                      ny, lowY, uppY);
  _origins->SetDirectory(plotDir);

  name.str(""); title.str("");
  name << _device->getName() << "OriginsX" << _nameSuffix;
  title << _device->getName() << " Origins X"
        << ";Origin position [" << _device->getSpaceUnit() << "]"
        << ";Tracks";
  _originsX = new TH1D(name.str().c_str(), title.str().c_str(),
                       nx, lowX, uppX);
  _originsX->SetDirectory(plotDir);

  name.str(""); title.str("");
  name << _device->getName() << "OriginsY" << _nameSuffix;
  title << _device->getName() << " Origins Y"
        << ";Origin position [" << _device->getSpaceUnit() << "]"
        << ";Tracks";
  _originsY = new TH1D(name.str().c_str(), title.str().c_str(),
                       ny, lowY, uppY);
  _originsY->SetDirectory(plotDir);

  name.str(""); title.str("");
  name << _device->getName() << "Chi2" << _nameSuffix;
  title << _device->getName() << " Chi^{2}"
        << ";Chi^{2} normalized to DOFs"
        << ";Tracks";
  _chi2 = new TH1D(name.str().c_str(), title.str().c_str(),
                   100, 0, 10);
  _chi2->SetDirectory(plotDir);

  const unsigned int maxClusters = _device->getNumSensors();
  const unsigned int clusterBins = maxClusters;

  name.str(""); title.str("");
  name << _device->getName() << "Clusters" << _nameSuffix;
  title << _device->getName() << " Number of Clusters"
        << ";Number of clusters"
        << ";Tracks";
  _numClusters = new TH1D(name.str().c_str(), title.str().c_str(),
                          clusterBins + 1, 0 - 0.5, maxClusters - 0.5 + 1);
  _numClusters->SetDirectory(plotDir);
}

}
