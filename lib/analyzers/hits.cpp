#include "hits.h"

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
                                  const Mechanics::Sensor& sensor)

{
  using namespace Utils;

  TDirectory* sub = Utils::makeDir(dir, "sensors/" + sensor.name() + "/hits");

  // time/value are digital values, bin 0 = [-0.5, 0.5)
  auto axCol = HistAxis::Integer(sensor.colRange(), "Hit column");
  auto axRow = HistAxis::Integer(sensor.rowRange(), "Hit row");
  auto axTs = HistAxis::Integer(sensor.timestampRange(), "Hit timestamp");
  auto axValue = HistAxis::Integer(sensor.valueRange(), "Hit value");

  m_nHits = makeH1(sub, "nhits", HistAxis::Integer(0, 64, "Hits / event"));
  m_rate = makeH1(sub, "rate", HistAxis{0.0, 1.0, 100, "Hits / pixel / event"});
  m_colRow = makeH2(sub, "colrow", axCol, axRow);
  m_timestamp = makeH1(sub, "timestamp", axTs);
  m_valueTimestamp = makeH2(sub, "timestamp-value", axValue, axTs);
  m_value = makeH1(sub, "value", axValue);
  m_meanTimestampMap = makeH2(sub, "mean_timestamp_map", axCol, axRow);
  m_meanValueMap = makeH2(sub, "mean_value_map", axCol, axRow);
  for (const auto& region : sensor.regions()) {
    TDirectory* rsub = Utils::makeDir(sub, region.name);
    RegionHists rh;
    rh.timestamp = makeH1(rsub, "timestamp", axTs);
    rh.valueTimestamp = makeH2(rsub, "timestamp-value", axValue, axTs);
    rh.value = makeH1(rsub, "value", axValue);
    m_regions.push_back(rh);
  }
}

void Analyzers::SensorHits::execute(const Storage::SensorEvent& sensorEvent)
{
  m_nHits->Fill(sensorEvent.numHits());

  for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
    const Storage::Hit& hit = sensorEvent.getHit(ihit);

    m_colRow->Fill(hit.col(), hit.row());
    m_timestamp->Fill(hit.timestamp());
    m_value->Fill(hit.value());
    m_valueTimestamp->Fill(hit.value(), hit.timestamp());
    m_meanTimestampMap->Fill(hit.col(), hit.row(), hit.timestamp());
    m_meanValueMap->Fill(hit.col(), hit.row(), hit.value());
    if (hit.hasRegion()) {
      m_regions[hit.region()].timestamp->Fill(hit.timestamp());
      m_regions[hit.region()].value->Fill(hit.value());
      m_regions[hit.region()].valueTimestamp->Fill(hit.value(),
                                                   hit.timestamp());
    }
  }
}

void Analyzers::SensorHits::finalize()
{
  // rescale rate histogram to available range
  auto numEvents = m_nHits->GetEntries();
  auto maxRate = m_colRow->GetMaximum() / numEvents;
  // ensure that the highest rate is still within the histogram limits
  m_rate->SetBins(m_rate->GetNbinsX(), 0,
                  std::nextafter(maxRate, std::numeric_limits<double>::max()));
  m_rate->Reset();
  // fill rate
  for (int ix = 1; ix <= m_colRow->GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= m_colRow->GetNbinsY(); ++iy) {
      // we only care about the hit rate for active pixels. otherwise we would
      // end up w/ a very large rate=0 bin in low statistic runs that obscurs
      // histogram entries we are interested in. in principle we could just
      // let the user rescale the histogram limits, but this is not a good
      // user experience.
      auto count = m_colRow->GetBinContent(ix, iy);
      if (0 < count) {
        m_rate->Fill(m_colRow->GetBinContent(ix, iy) / numEvents);
      }
    }
  }
  // scale from integrated time/value to mean
  m_meanTimestampMap->Divide(m_colRow);
  m_meanValueMap->Divide(m_colRow);
}

Analyzers::Hits::Hits(TDirectory* dir, const Mechanics::Device& device)
{
  for (auto isensor : device.sensorIds()) {
    m_sensors.emplace_back(dir, device.getSensor(isensor));
  }
}

std::string Analyzers::Hits::name() const { return "Hits"; }

void Analyzers::Hits::execute(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    m_sensors[isensor].execute(event.getSensorEvent(isensor));
  }
}

void Analyzers::Hits::finalize()
{
  for (auto& sensor : m_sensors) {
    sensor.finalize();
  }
}
