#include "hitinfo.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "mechanics/sensor.h"
#include "processors/processors.h"
#include "storage/cluster.h"
#include "storage/event.h"
#include "storage/hit.h"
#include "storage/plane.h"
#include "storage/track.h"

namespace Analyzers {

void HitInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (unsigned int nplane = 0; nplane < event->numPlanes(); nplane++) {
    const Storage::Plane* plane = event->getPlane(nplane);

    for (unsigned int nhit = 0; nhit < plane->numHits(); nhit++) {
      const Storage::Hit* hit = plane->getHit(nhit);

      // Check if the hit passes the cuts
      if (!checkCuts(hit))
        continue;

      _lvl1.at(nplane)->Fill(hit->getTiming());
      _tot.at(nplane)->Fill(hit->getValue());
      _totMap.at(nplane)->Fill(hit->getPixX(), hit->getPixY(), hit->getValue());
      _timingMap.at(nplane)->Fill(hit->getPixX(), hit->getPixY(),
                                  hit->getTiming());
      _MapCnt.at(nplane)->Fill(hit->getPixX(), hit->getPixY());
    }
  }
}

void HitInfo::postProcessing()
{
  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++) {
    TH2D* map = _totMap.at(nsens);
    TH2D* count = _MapCnt.at(nsens);
    for (Int_t x = 1; x <= map->GetNbinsX(); x++) {
      for (Int_t y = 1; y <= map->GetNbinsY(); y++) {
        const double average =
            map->GetBinContent(x, y) / count->GetBinContent(x, y);
        map->SetBinContent(x, y, average);
      }
    }

    TH2D* timingMap = _timingMap.at(nsens);

    for (Int_t x = 1; x <= timingMap->GetNbinsX(); x++) {
      for (Int_t y = 1; y <= timingMap->GetNbinsY(); y++) {
        const double average =
            timingMap->GetBinContent(x, y) / count->GetBinContent(x, y);
        timingMap->SetBinContent(x, y, average);
      }
    }
  }
}

HitInfo::HitInfo(const Mechanics::Device* device,
                 TDirectory* dir,
                 const char* suffix,
                 unsigned int lvl1Bins,
                 unsigned int totBins)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(device, dir, suffix, "HitInfo")
    , _lvl1Bins(lvl1Bins)
    , _totBins(totBins)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("HitInfo");

  std::stringstream name;  // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++) {
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);

    name.str("");
    title.str("");
    name << sensor->getName() << "Timing" << _nameSuffix;
    title << sensor->getName() << " Level 1 Accept"
          << ";Level 1 bin number"
          << ";Hits";
    TH1D* lvl1 = new TH1D(name.str().c_str(), title.str().c_str(), _lvl1Bins,
                          -0.5, _lvl1Bins - 0.5);
    lvl1->SetDirectory(plotDir);
    _lvl1.push_back(lvl1);

    name.str("");
    title.str("");
    name << sensor->getName() << "ToT" << _nameSuffix;
    title << sensor->getName() << " ToT Distribution"
          << ";ToT bin number"
          << ";Hits";
    TH1D* tot = new TH1D(name.str().c_str(), title.str().c_str(), _totBins,
                         -0.5, _totBins - 0.5);
    tot->SetDirectory(plotDir);
    _tot.push_back(tot);

    name.str("");
    title.str("");
    name << sensor->getName() << "ToTMap" << _nameSuffix;
    title << sensor->getName() << " ToT Map"
          << ";X pixel"
          << ";Y pixel"
          << ";Average ToT";
    TH2D* totMap = new TH2D(name.str().c_str(), title.str().c_str(),
                            sensor->getNumX(), -0.5, sensor->getNumX() - 0.5,
                            sensor->getNumY(), -0.5, sensor->getNumY() - 0.5);
    totMap->SetDirectory(plotDir);
    _totMap.push_back(totMap);

    name.str("");
    title.str("");
    name << sensor->getName() << "TimingMap" << _nameSuffix;
    title << sensor->getName() << " Timing Map"
          << ";X pixel"
          << ";Y pixel"
          << ";Average Lv1 Timing";

    TH2D* timingMap =
        new TH2D(name.str().c_str(), title.str().c_str(), sensor->getNumX(),
                 -0.5, sensor->getNumX() - 0.5, sensor->getNumY(), -0.5,
                 sensor->getNumY() - 0.5);
    timingMap->SetDirectory(plotDir);
    _timingMap.push_back(timingMap);

    name.str("");
    title.str("");
    name << sensor->getName() << "ToTMapCnt" << _nameSuffix;
    title << sensor->getName() << " ToT Map Counter";
    TH2D* MapCnt = new TH2D(name.str().c_str(), title.str().c_str(),
                            sensor->getNumX(), -0.5, sensor->getNumX() - 0.5,
                            sensor->getNumY(), -0.5, sensor->getNumY() - 0.5);
    MapCnt->SetDirectory(0);
    _MapCnt.push_back(MapCnt);
  }
}
}
