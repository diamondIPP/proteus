#include "hitinfo.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::HitInfo::HitInfo(const Mechanics::Device* device,
                            TDirectory* dir,
                            const int timeMax,
                            const int valueMax)
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

std::string Analyzers::HitInfo::name() const { return "HitInfo"; }

void Analyzers::HitInfo::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < m_hists.size(); ++isensor) {
    SensorHists& hists = m_hists[isensor];

    const Storage::Plane& plane = *event.getPlane(isensor);
    for (Index ihit = 0; ihit < plane.numHits(); ++ihit) {
      const Storage::Hit& hit = *plane.getHit(ihit);

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

void Analyzers::HitInfo::finalize()
{
  // scale from integrated time/value to mean
  for (auto& hists : m_hists) {
    hists.meanTimeMap->Divide(hists.hitMap);
    hists.meanValueMap->Divide(hists.hitMap);
  }
}
