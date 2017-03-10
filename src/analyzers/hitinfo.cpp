#include "hitinfo.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::HitInfo::HitInfo(const Mechanics::Device* device,
                            TDirectory* dir,
                            const char* suffix,
                            const int timeMax,
                            const int valueMax)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(device, dir, suffix, "HitInfo")
{
  using namespace Utils;

  assert(device && "Analyzer: can't initialize with null device");

  TDirectory* sub = Utils::makeDir(dir, "HitInfo");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);
    const auto& area = sensor.sensitiveAreaPixel();
    auto name = [&](const std::string suffix) {
      return sensor.name() + '-' + suffix;
    };
    auto makeRegionHists = [&](std::string name) {
      std::string prefix = sensor.name() + '-';
      if (!name.empty()) {
        prefix += name;
        prefix += '-';
      }
      RegionHists h;
      h.time = makeH1(sub, prefix + "Time", HistAxis(0, timeMax, "Hit time"));
      h.value =
          makeH1(sub, prefix + "Value", HistAxis(0, valueMax, "Hit value"));
      return h;
    };

    HistAxis axCol(area.interval(0), area.length(0), "Hit column");
    HistAxis axRow(area.interval(1), area.length(1), "Hit row");
    SensorHists hists;
    // hitmap is only for internal normalization
    hists.hitMap = makeTransientH2(axCol, axRow);
    hists.meanTimeMap = makeH2(sub, name("MeanTimeMap"), axCol, axRow);
    hists.meanValueMap = makeH2(sub, name("MeanValueMap"), axCol, axRow);
    hists.whole = makeRegionHists(std::string());
    for (const auto& region : sensor.regions()) {
      hists.regions.push_back(makeRegionHists(region.name));
    }
    m_hists.push_back(std::move(hists));
  }
}

void Analyzers::HitInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (Index isensor = 0; isensor < event->numPlanes(); isensor++) {
    const Storage::Plane& plane = *event->getPlane(isensor);
    const SensorHists& hists = m_hists.at(isensor);

    for (Index ihit = 0; ihit < plane.numHits(); ihit++) {
      const Storage::Hit& hit = *plane.getHit(ihit);

      // Check if the hit passes the cuts
      if (!checkCuts(&hit))
        continue;

      hists.hitMap->Fill(hit.col(), hit.row());
      hists.meanTimeMap->Fill(hit.col(), hit.row(), hit.time());
      hists.meanValueMap->Fill(hit.col(), hit.row(), hit.value());
      hists.whole.time->Fill(hit.time());
      hists.whole.value->Fill(hit.value());
      if (hit.region() != kInvalidIndex) {
        hists.regions[hit.region()].time->Fill(hit.time());
        hists.regions[hit.region()].value->Fill(hit.value());
      }
    }
  }
}

void Analyzers::HitInfo::postProcessing()
{
  // scale from integrated time/value to mean
  for (auto& hists : m_hists) {
    hists.meanTimeMap->Divide(hists.hitMap);
    hists.meanValueMap->Divide(hists.hitMap);
  }
}
