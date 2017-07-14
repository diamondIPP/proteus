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

Analyzers::SensorHits::SensorHits(TDirectory* dir,
                                  const Mechanics::Sensor& sensor,
                                  const int timeMax,
                                  const int valueMax)

{
  using namespace Utils;

  const auto& area = sensor.sensitiveAreaPixel();

  TDirectory* sub = Utils::makeDir(dir, sensor.name() + "/hits");

  HistAxis axCol(area.interval(0), area.length(0), "Hit column");
  HistAxis axRow(area.interval(1), area.length(1), "Hit row");
  HistAxis axTime(0, timeMax, "Hit value");
  HistAxis axValue(0, valueMax, "Hit time");

  m_nHits = makeH1(sub, "nhits", HistAxis{0, 64, "Hits / event"});
  m_rate = makeH1(sub, "rate", HistAxis{0.0, 1.0, 128, "Hits / pixel / event"});
  m_pos = makeH2(sub, "pos", axCol, axRow);
  m_time = makeH1(sub, "time", axTime);
  m_value = makeH1(sub, "value", axValue);
  m_meanTimeMap = makeH2(sub, "mean_time_map", axCol, axRow);
  m_meanValueMap = makeH2(sub, "mean_value_map", axCol, axRow);
  for (const auto& region : sensor.regions()) {
    TDirectory* rsub = Utils::makeDir(sub, region.name);
    RegionHists rh;
    rh.time = makeH1(rsub, "time", axTime);
    rh.value = makeH1(rsub, "value", axValue);
    m_regions.push_back(rh);
  }
}

void Analyzers::SensorHits::analyze(const Storage::Plane& sensorEvent)
{
  m_nHits->Fill(sensorEvent.numHits());

  for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
    const Storage::Hit& hit = *sensorEvent.getHit(ihit);

    m_pos->Fill(hit.col(), hit.row());
    m_time->Fill(hit.time());
    m_value->Fill(hit.value());
    m_meanTimeMap->Fill(hit.col(), hit.row(), hit.time());
    m_meanValueMap->Fill(hit.col(), hit.row(), hit.value());
    if (hit.region() != kInvalidIndex) {
      m_regions[hit.region()].time->Fill(hit.time());
      m_regions[hit.region()].value->Fill(hit.value());
    }
  }
}

void Analyzers::SensorHits::finalize()
{
  // rescale rate histogram to available range
  auto numEvents = m_nHits->GetEntries();
  m_rate->SetBins(m_rate->GetNbinsX(), 0, m_pos->GetMaximum() / numEvents);
  m_rate->Reset();
  // fill rate
  for (int ix = 1; ix <= m_pos->GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= m_pos->GetNbinsY(); ++iy) {
      auto count = m_pos->GetBinContent(ix, iy);
      if (count != 0)
        m_rate->Fill(count / numEvents);
    }
  }
  // scale from integrated time/value to mean
  m_meanTimeMap->Divide(m_pos);
  m_meanValueMap->Divide(m_pos);
}

Analyzers::Hits::Hits(const Mechanics::Device* device,
                      TDirectory* dir,
                      const int timeMax,
                      const int valueMax)
{
  assert(device && "Analyzer: can't initialize with null device");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor)
    m_sensors.emplace_back(dir, *device->getSensor(isensor), timeMax, valueMax);
}

std::string Analyzers::Hits::name() const { return "Hits"; }

void Analyzers::Hits::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < event.numPlanes(); ++isensor) {
    m_sensors[isensor].analyze(*event.getPlane(isensor));
  }
}

void Analyzers::Hits::finalize()
{
  for (auto& sensor : m_sensors)
    sensor.finalize();
}
