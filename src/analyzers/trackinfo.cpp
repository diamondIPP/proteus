#include "trackinfo.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"

void Analyzers::TrackInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (unsigned int ntrack = 0; ntrack < event->getNumTracks(); ntrack++) {
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
  }
}

void Analyzers::TrackInfo::postProcessing() {}

Analyzers::TrackInfo::TrackInfo(const Mechanics::Device* device,
                                TDirectory* dir,
                                const char* suffix,
                                double resWidth,
                                double maxSlope,
                                double increaseArea)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(device, dir, suffix, "TrackInfo")
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("TrackInfo");

  std::stringstream name;  // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo
                           // std::stringstream xAxisTitle;

  // Use DUT 0 as a reference sensor for plots axis
  assert(_device->getNumSensors() &&
         "TrackInfo: expects device to have sensors");
  const Mechanics::Sensor* sensor = _device->getSensor(0);

  const double deviceLength =
      device->getSensor(device->getNumSensors() - 1)->getOffZ() -
      device->getSensor(0)->getOffZ();
  const double maxSlopeX = maxSlope * sensor->getPosPitchX() / deviceLength;
  const double maxSlopeY = maxSlope * sensor->getPosPitchY() / deviceLength;

  name.str("");
  title.str("");
  name << _device->getName() << "SlopesX" << _nameSuffix;
  title << _device->getName() << " Slopes X"
        << ";Slope [radians]"
        << ";Tracks";
  _slopesX = new TH1D(name.str().c_str(), title.str().c_str(), 100, -maxSlopeX,
                      maxSlopeX);
  _slopesX->SetDirectory(plotDir);

  name.str("");
  title.str("");
  name << _device->getName() << "SlopesY" << _nameSuffix;
  title << _device->getName() << " Slopes Y"
        << ";Slope [radians]"
        << ";Tracks";
  _slopesY = new TH1D(name.str().c_str(), title.str().c_str(), 100, -maxSlopeY,
                      maxSlopeY);
  _slopesY->SetDirectory(plotDir);

  const unsigned int nx = increaseArea * sensor->getPosNumX();
  const unsigned int ny = increaseArea * sensor->getPosNumY();
  const double lowX =
      sensor->getOffX() - increaseArea * sensor->getPosSensitiveX() / 2.0;
  const double uppX =
      sensor->getOffX() + increaseArea * sensor->getPosSensitiveX() / 2.0;
  const double lowY =
      sensor->getOffY() - increaseArea * sensor->getPosSensitiveY() / 2.0;
  const double uppY =
      sensor->getOffY() + increaseArea * sensor->getPosSensitiveY() / 2.0;

  name.str("");
  title.str("");
  name << _device->getName() << "Origins" << _nameSuffix;
  title << _device->getName() << " Origins"
        << ";Origin position X [" << _device->getSpaceUnit() << "]"
        << ";Origin position Y [" << _device->getSpaceUnit() << "]"
        << ";Tracks";
  _origins = new TH2D(name.str().c_str(), title.str().c_str(), nx, lowX, uppX,
                      ny, lowY, uppY);
  _origins->SetDirectory(plotDir);

  name.str("");
  title.str("");
  name << _device->getName() << "OriginsX" << _nameSuffix;
  title << _device->getName() << " Origins X"
        << ";Origin position [" << _device->getSpaceUnit() << "]"
        << ";Tracks";
  _originsX = new TH1D(name.str().c_str(), title.str().c_str(), nx, lowX, uppX);
  _originsX->SetDirectory(plotDir);

  name.str("");
  title.str("");
  name << _device->getName() << "OriginsY" << _nameSuffix;
  title << _device->getName() << " Origins Y"
        << ";Origin position [" << _device->getSpaceUnit() << "]"
        << ";Tracks";
  _originsY = new TH1D(name.str().c_str(), title.str().c_str(), ny, lowY, uppY);
  _originsY->SetDirectory(plotDir);

  name.str("");
  title.str("");
  name << _device->getName() << "Chi2" << _nameSuffix;
  title << _device->getName() << " Chi^{2}"
        << ";Chi^{2} normalized to DOFs"
        << ";Tracks";
  _chi2 = new TH1D(name.str().c_str(), title.str().c_str(), 100, 0, 10);
  _chi2->SetDirectory(plotDir);

  const unsigned int maxClusters = _device->getNumSensors();
  const unsigned int clusterBins = maxClusters;

  name.str("");
  title.str("");
  name << _device->getName() << "Clusters" << _nameSuffix;
  title << _device->getName() << " Number of Clusters"
        << ";Number of clusters"
        << ";Tracks";
  _numClusters = new TH1D(name.str().c_str(), title.str().c_str(),
                          clusterBins + 1, 0 - 0.5, maxClusters - 0.5 + 1);
  _numClusters->SetDirectory(plotDir);
}
