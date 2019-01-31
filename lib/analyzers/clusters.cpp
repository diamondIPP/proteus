#include "clusters.h"

#include <cassert>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::SensorClusters::SensorClusters(TDirectory* dir,
                                          const Mechanics::Sensor& sensor,
                                          const int sizeMax,
                                          const int binsUncertainty)
{
  using namespace Utils;

  auto makeAreaHists = [&](const Mechanics::Sensor& sensor,
                           const Mechanics::Sensor::DigitalArea& area,
                           TDirectory* sub) {
    auto ts = sensor.timestampRange();
    auto value = sensor.valueRange();

    auto axTimestamp = HistAxis::Integer(ts, "Cluster timestamp");
    // increase value range since cluster value is usually additive
    auto axValue = HistAxis::Integer(
        value.min(), (value.max() + value.length()), "Cluster value");
    auto axSize = HistAxis::Integer(1, sizeMax, "Cluster size");
    auto axSizeCol = HistAxis::Integer(1, sizeMax, "Cluster column size");
    auto axSizeRow = HistAxis::Integer(1, sizeMax, "Cluster row size");
    auto axUnU = HistAxis(0, sensor.pitchCol() / 2, binsUncertainty,
                          "Cluster uncertainty u");
    auto axUnV = HistAxis(0, sensor.pitchRow() / 2, binsUncertainty,
                          "Cluster uncertainty v");
    auto axUnS = HistAxis(0, sensor.pitchTimestamp() / 2, binsUncertainty,
                          "Cluster uncertainty time");
    auto axHitTimestamp = HistAxis::Integer(ts, "Hit timestamp");
    // maximum time differences computable, max value is exclusive
    auto axHitTimedelta =
        HistAxis::Integer(ts.min() - ts.max() + 1, ts.max() - ts.min(),
                          "Hit - cluster timestamp");
    auto axHitValue = HistAxis::Integer(value, "Hit value");

    AreaHists hs;
    hs.timestamp = makeH1(sub, "timestamp", axTimestamp);
    hs.sizeTimestamp = makeH2(sub, "timestamp-size", axSize, axTimestamp);
    hs.value = makeH1(sub, "value", axValue);
    hs.sizeValue = makeH2(sub, "value-size", axSize, axValue);
    hs.size = makeH1(sub, "size", axSize);
    hs.sizeSizeCol = makeH2(sub, "size_col-size", axSize, axSizeCol);
    hs.sizeSizeRow = makeH2(sub, "size_row-size", axSize, axSizeRow);
    hs.sizeColSizeRow = makeH2(sub, "size_row-size_col", axSizeCol, axSizeRow);
    hs.uncertaintyU = makeH1(sub, "uncertainty_u", axUnU);
    hs.uncertaintyV = makeH1(sub, "uncertainty_v", axUnV);
    hs.uncertaintyTime = makeH1(sub, "uncertainty_time", axUnS);
    hs.sizeHitTimestamp =
        makeH2(sub, "hit_timestamp-size", axSize, axHitTimestamp);
    hs.hitTimedelta = makeH1(sub, "hit_timedelta", axHitTimedelta);
    hs.sizeHitTimedelta =
        makeH2(sub, "hit_timedelta-size", axSize, axHitTimedelta);
    hs.sizeHitValue = makeH2(sub, "hit_value-size", axSize, axHitValue);

    return hs;
  };

  TDirectory* sub = makeDir(dir, "sensors/" + sensor.name() + "/clusters");
  m_nClusters =
      makeH1(sub, "nclusters", HistAxis::Integer(0, 64, "Clusters / event"));
  m_rate =
      makeH1(sub, "rate", HistAxis{0, 1.0, 128, "Clusters / pixel / event"});
  m_colRow =
      makeH2(sub, "colrow",
             HistAxis::Integer(sensor.colRange(), "Cluster column position"),
             HistAxis::Integer(sensor.rowRange(), "Cluster row position"));
  m_whole = makeAreaHists(sensor, sensor.colRowArea(), sub);
  for (const auto& region : sensor.regions()) {
    TDirectory* rsub = Utils::makeDir(sub, region.name);
    m_regions.push_back(makeAreaHists(sensor, region.colRow, rsub));
  }
}

void Analyzers::SensorClusters::execute(const Storage::SensorEvent& sensorEvent)
{
  auto fill = [](const Storage::Cluster& cluster, AreaHists& hists) {
    hists.timestamp->Fill(cluster.timestamp());
    hists.value->Fill(cluster.value());
    hists.size->Fill(cluster.size());
    hists.sizeTimestamp->Fill(cluster.size(), cluster.timestamp());
    hists.sizeValue->Fill(cluster.size(), cluster.value());
    hists.sizeSizeCol->Fill(cluster.size(), cluster.sizeCol());
    hists.sizeSizeRow->Fill(cluster.size(), cluster.sizeRow());
    hists.sizeColSizeRow->Fill(cluster.sizeCol(), cluster.sizeRow());
    auto stdev = extractStdev(cluster.positionCov());
    hists.uncertaintyU->Fill(stdev[kU]);
    hists.uncertaintyV->Fill(stdev[kV]);
    hists.uncertaintyTime->Fill(stdev[kS]);
    for (const Storage::Hit& hit : cluster.hits()) {
      hists.sizeHitTimestamp->Fill(cluster.size(), hit.timestamp());
      auto timedelta = hit.timestamp() - cluster.timestamp();
      hists.hitTimedelta->Fill(timedelta);
      hists.sizeHitTimedelta->Fill(cluster.size(), timedelta);
      hists.sizeHitValue->Fill(cluster.size(), hit.value());
    }
  };

  m_nClusters->Fill(sensorEvent.numClusters());
  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = sensorEvent.getCluster(icluster);
    m_colRow->Fill(cluster.col(), cluster.row());
    fill(cluster, m_whole);
    if (cluster.hasRegion()) {
      fill(cluster, m_regions[cluster.region()]);
    }
  }
}

void Analyzers::SensorClusters::finalize()
{
  // rescale rate histogram to available range
  auto numEvents = m_nClusters->GetEntries();
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
        m_rate->Fill(count / numEvents);
      }
    }
  }
}

Analyzers::Clusters::Clusters(TDirectory* dir,
                              const Mechanics::Device& device,
                              const int sizeMax,
                              const int binsUncertainty)
{
  for (auto isensor : device.sensorIds()) {
    m_sensors.emplace_back(dir, device.getSensor(isensor), sizeMax,
                           binsUncertainty);
  }
}

std::string Analyzers::Clusters::name() const { return "ClusterInfo"; }

void Analyzers::Clusters::execute(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < m_sensors.size(); ++isensor) {
    m_sensors[isensor].execute(event.getSensorEvent(isensor));
  }
}

void Analyzers::Clusters::finalize()
{
  for (auto& sensor : m_sensors) {
    sensor.finalize();
  }
}
