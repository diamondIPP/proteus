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
                                          const int timeMax,
                                          const int valueMax,
                                          const int sizeMax,
                                          const int binsUncertainty)
{
  using namespace Utils;

  auto makeAreaHists = [&](const Mechanics::Sensor& sensor,
                           const Mechanics::Sensor::Area& area,
                           TDirectory* sub) {
    HistAxis axCol(area.interval(0), area.length(0), "Cluster column position");
    HistAxis axRow(area.interval(1), area.length(1), "Cluster row position");
    HistAxis axTime(0, timeMax, "Cluster time");
    HistAxis axValue(0, valueMax, "Cluster value");
    HistAxis axSize(1, sizeMax, "Cluster size");
    HistAxis axSizeCol(1, sizeMax, "Cluster column size");
    HistAxis axSizeRow(1, sizeMax, "Cluster row size");
    HistAxis axUnU(0, sensor.pitchCol() / 2, binsUncertainty,
                   "Cluster uncertainty u");
    HistAxis axUnV(0, sensor.pitchRow() / 2, binsUncertainty,
                   "Cluster uncertainty v");
    HistAxis axHitCol(area.interval(0), area.length(0),
                      "Cluster hit column position");
    HistAxis axHitRow(area.interval(1), area.length(1),
                      "Cluster hit row position");
    HistAxis axHitTime(0, timeMax, "Hit time");
    HistAxis axHitValue(0, valueMax, "Hit value");

    AreaHists hs;
    hs.pos = makeH2(sub, "pos", axCol, axRow);
    hs.time = makeH1(sub, "time", axTime);
    hs.value = makeH1(sub, "value", axValue);
    hs.size = makeH1(sub, "size", axSize);
    hs.sizeSizeCol = makeH2(sub, "size_col-size", axSize, axSizeCol);
    hs.sizeSizeRow = makeH2(sub, "size_row-size", axSize, axSizeRow);
    hs.sizeColSizeRow = makeH2(sub, "size_row-size_col", axSizeCol, axSizeRow);
    hs.sizeValue = makeH2(sub, "value-size", axSize, axValue);
    hs.uncertaintyU = makeH1(sub, "uncertainty_u", axUnU);
    hs.uncertaintyV = makeH1(sub, "uncertainty_v", axUnV);
    hs.hitPos = makeH2(sub, "hit_pos", axHitCol, axHitRow);
    hs.sizeHitTime = makeH2(sub, "hit_time-size", axSize, axHitTime);
    hs.sizeHitValue = makeH2(sub, "hit_value-size", axSize, axHitValue);
    hs.hitValueHitTime =
        makeH2(sub, "hit_time-hit_value", axHitValue, axHitTime);
    return hs;
  };

  TDirectory* sub = makeDir(dir, "sensors/" + sensor.name() + "/clusters");

  m_nClusters = makeH1(sub, "nclusters", HistAxis{0, 64, "Clusters / event"});
  m_rate =
      makeH1(sub, "rate", HistAxis{0, 1.0, 128, "Clusters / pixel / event"});
  m_whole = makeAreaHists(sensor, sensor.sensitiveAreaPixel(), sub);
  for (const auto& region : sensor.regions()) {
    TDirectory* rsub = Utils::makeDir(sub, region.name);
    m_regions.push_back(makeAreaHists(sensor, region.areaPixel, rsub));
  }
}

void Analyzers::SensorClusters::execute(const Storage::SensorEvent& sensorEvent)
{
  auto fill = [](const Storage::Cluster& cluster, AreaHists& hists) {
    auto pos = cluster.posPixel();
    hists.pos->Fill(pos.x(), pos.y());
    hists.time->Fill(cluster.time());
    hists.value->Fill(cluster.value());
    hists.size->Fill(cluster.size());
    hists.sizeSizeCol->Fill(cluster.size(), cluster.sizeCol());
    hists.sizeSizeRow->Fill(cluster.size(), cluster.sizeRow());
    hists.sizeColSizeRow->Fill(cluster.sizeCol(), cluster.sizeRow());
    hists.sizeValue->Fill(cluster.size(), cluster.value());
    hists.uncertaintyU->Fill(std::sqrt(cluster.covLocal()(0, 0)));
    hists.uncertaintyV->Fill(std::sqrt(cluster.covLocal()(1, 1)));
    for (const Storage::Hit& hit : cluster.hits()) {
      hists.hitPos->Fill(hit.col(), hit.row());
      hists.sizeHitTime->Fill(cluster.size(), hit.time());
      hists.sizeHitValue->Fill(cluster.size(), hit.value());
      hists.hitValueHitTime->Fill(hit.value(), hit.time());
    }
  };

  m_nClusters->Fill(sensorEvent.numClusters());
  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = sensorEvent.getCluster(icluster);
    fill(cluster, m_whole);
    if (cluster.hasRegion())
      fill(cluster, m_regions[cluster.region()]);
  }
}

void Analyzers::SensorClusters::finalize()
{
  // rescale rate histogram to available range
  auto numEvents = m_nClusters->GetEntries();
  m_rate->SetBins(m_rate->GetNbinsX(), 0,
                  m_whole.pos->GetMaximum() / numEvents);
  m_rate->Reset();
  // fill rate
  for (int ix = 1; ix <= m_whole.pos->GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= m_whole.pos->GetNbinsY(); ++iy) {
      auto count = m_whole.pos->GetBinContent(ix, iy);
      if (count != 0)
        m_rate->Fill(count / numEvents);
    }
  }
}

Analyzers::Clusters::Clusters(TDirectory* dir,
                              const Mechanics::Device& device,
                              const int sizeMax,
                              const int timeMax,
                              const int valueMax,
                              const int binsUncertainty)
{
  using namespace Utils;

  for (auto isensor : device.sensorIds())
    m_sensors.emplace_back(dir, *device.getSensor(isensor), timeMax, valueMax,
                           sizeMax, binsUncertainty);
}

std::string Analyzers::Clusters::name() const { return "ClusterInfo"; }

void Analyzers::Clusters::execute(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < m_sensors.size(); ++isensor)
    m_sensors[isensor].execute(event.getSensorEvent(isensor));
}

void Analyzers::Clusters::finalize()
{
  for (auto& sensor : m_sensors)
    sensor.finalize();
}
