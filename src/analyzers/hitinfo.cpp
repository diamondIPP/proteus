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

  for (Index sensorId = 0; sensorId < device->numSensors(); ++sensorId) {
    const Mechanics::Sensor& sensor = *device->getSensor(sensorId);
    const auto& area = sensor.sensitiveAreaPixel();
    auto name = [&](const std::string suffix) {
      return sensor.name() + '-' + suffix;
    };

    HistAxis axCol(area.axes[0], area.axes[0].length(), "Hit colum");
    HistAxis axRow(area.axes[1], area.axes[1].length(), "Hit row");
    HistAxis axTime(0, timeMax, "Hit time");
    HistAxis axValue(0, valueMax, "Hit value");

    Hists hists;
    hists.pixels = makeH2(sub, name("Pixels"), axCol, axRow);
    hists.time = makeH1(sub, name("Time"), axTime);
    hists.value = makeH1(sub, name("Value"), axValue);
    hists.timeMap = makeH2(sub, name("TimeMap"), axCol, axRow);
    hists.valueMap = makeH2(sub, name("ValueMap"), axCol, axRow);
    m_hists.push_back(hists);
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

  for (Index sensorId = 0; sensorId < event->numPlanes(); sensorId++) {
    const Storage::Plane& plane = *event->getPlane(sensorId);
    const Hists& hists = m_hists.at(sensorId);

    for (Index ihit = 0; ihit < plane.numHits(); ihit++) {
      const Storage::Hit& hit = *plane.getHit(ihit);

      // Check if the hit passes the cuts
      if (!checkCuts(&hit))
        continue;

      hists.pixels->Fill(hit.col(), hit.row());
      hists.time->Fill(hit.time());
      hists.value->Fill(hit.value());
      hists.timeMap->Fill(hit.col(), hit.row(), hit.time());
      hists.valueMap->Fill(hit.col(), hit.row(), hit.value());
    }
  }
}

void Analyzers::HitInfo::postProcessing()
{
  for (auto hists = m_hists.begin(); hists != m_hists.end(); ++hists) {
    // scale time and tot map to pixel hit count
    for (Int_t x = 1; x <= hists->pixels->GetNbinsX(); ++x) {
      for (Int_t y = 1; y <= hists->pixels->GetNbinsY(); ++y) {
        const double count = hists->pixels->GetBinContent(x, y);
        const double time = hists->timeMap->GetBinContent(x, y);
        hists->timeMap->SetBinContent(x, y, time / count);
        const double value = hists->valueMap->GetBinContent(x, y);
        hists->valueMap->SetBinContent(x, y, value / count);
      }
    }
  }
}
