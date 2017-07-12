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

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);
    const auto& area = sensor.sensitiveAreaPixel();

    TDirectory* sub = Utils::makeDir(dir, sensor.name() + "/hits");

    HistAxis axCol(area.interval(0), area.length(0), "Hit column");
    HistAxis axRow(area.interval(1), area.length(1), "Hit row");
    HistAxis axTime(0, timeMax, "Hit value");
    HistAxis axValue(0, valueMax, "Hit time");

    SensorHists sh;
    sh.nHits = makeH1(sub, "nhits", HistAxis{0, 64, "Hits / event"});
    sh.hitMap = makeH2(sub, "hit_map", axCol, axRow);
    sh.time = makeH1(sub, "time", axTime);
    sh.value = makeH1(sub, "value", axValue);
    sh.meanTimeMap = makeH2(sub, "mean_time_map", axCol, axRow);
    sh.meanValueMap = makeH2(sub, "mean_value_map", axCol, axRow);
    for (const auto& region : sensor.regions()) {
      TDirectory* rsub = Utils::makeDir(sub, region.name);
      RegionHists rh;
      rh.time = makeH1(rsub, "time", axTime);
      rh.value = makeH1(rsub, "value", axValue);
      sh.regions.push_back(std::move(rh));
    }

    m_hists.push_back(std::move(sh));
  }
}

std::string Analyzers::HitInfo::name() const { return "HitInfo"; }

void Analyzers::HitInfo::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < m_hists.size(); ++isensor) {
    SensorHists& hists = m_hists[isensor];
    const Storage::Plane& plane = *event.getPlane(isensor);

    hists.nHits->Fill(plane.numHits());

    for (Index ihit = 0; ihit < plane.numHits(); ++ihit) {
      const Storage::Hit& hit = *plane.getHit(ihit);

      hists.hitMap->Fill(hit.col(), hit.row());
      hists.time->Fill(hit.time());
      hists.value->Fill(hit.value());
      hists.meanTimeMap->Fill(hit.col(), hit.row(), hit.time());
      hists.meanValueMap->Fill(hit.col(), hit.row(), hit.value());
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
