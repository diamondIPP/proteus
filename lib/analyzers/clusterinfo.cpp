#include "clusterinfo.h"

#include <cassert>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

void Analyzers::ClusterInfo::ClusterHists::fill(const Storage::Cluster& cluster)
{
  auto cluPix = cluster.posPixel();
  pos->Fill(cluPix.x(), cluPix.y());
  value->Fill(cluster.value());
  size->Fill(cluster.size());
  sizeSizeCol->Fill(cluster.size(), cluster.sizeCol());
  sizeSizeRow->Fill(cluster.size(), cluster.sizeRow());
  sizeColSizeRow->Fill(cluster.sizeCol(), cluster.sizeRow());
  sizeValue->Fill(cluster.size(), cluster.value());
  uncertaintyU->Fill(std::sqrt(cluster.covLocal()(0, 0)));
  uncertaintyV->Fill(std::sqrt(cluster.covLocal()(1, 1)));
  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit& hit = *cluster.getHit(ihit);
    auto hitPix = hit.posPixel();
    hitPos->Fill(hitPix.x(), hitPix.y());
    sizeHitTime->Fill(cluster.size(), hit.time());
    sizeHitValue->Fill(cluster.size(), hit.value());
    hitValueHitTime->Fill(hit.value(), hit.time());
  }
}

void Analyzers::ClusterInfo::SensorHists::fill(const Storage::Cluster& cluster)
{
  whole.fill(cluster);
  if (cluster.region() != kInvalidIndex)
    regions[cluster.region()].fill(cluster);
}

Analyzers::ClusterInfo::ClusterInfo(const Mechanics::Device* device,
                                    TDirectory* dir,
                                    const int sizeMax,
                                    const int timeMax,
                                    const int valueMax,
                                    const int binsUncertainty)
{
  using namespace Utils;

  assert(device && "Analyzer: can't initialize with null device");

  auto makeClusterHists = [&](const Mechanics::Sensor& sensor,
                              const Mechanics::Sensor::Area& area,
                              TDirectory* sub) {
    HistAxis axClusterCol(area.interval(0), area.length(0), "Cluster column");
    HistAxis axClusterRow(area.interval(1), area.length(1), "Cluster row");
    HistAxis axValue(0, valueMax, "Cluster value");
    HistAxis axSize(1, sizeMax, "Cluster size");
    HistAxis axSizeCol(1, sizeMax, "Cluster column size");
    HistAxis axSizeRow(1, sizeMax, "Cluster row size");
    HistAxis axUnU(0, sensor.pitchCol() / 2, binsUncertainty,
                   "Cluster uncertainty u");
    HistAxis axUnV(0, sensor.pitchRow() / 2, binsUncertainty,
                   "Cluster uncertainty v");
    HistAxis axHitCol(area.interval(0), area.length(0), "Hit column");
    HistAxis axHitRow(area.interval(1), area.length(1), "Hit row");
    HistAxis axHitTime(0, timeMax, "Hit time");
    HistAxis axHitValue(0, valueMax, "Hit value");

    ClusterHists hs;
    hs.pos = makeH2(sub, "pos", axClusterCol, axClusterRow);
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

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);
    TDirectory* sub = Utils::makeDir(dir, sensor.name() + "/clusters");

    SensorHists hists;
    hists.nClusters =
        makeH1(sub, "nclusters", HistAxis{0, 64, "Clusters / event"});
    hists.rate =
        makeH1(sub, "rate", HistAxis{0, 1.0, 128, "Clusters / pixel / event"});
    hists.whole = makeClusterHists(sensor, sensor.sensitiveAreaPixel(), sub);
    for (const auto& region : sensor.regions()) {
      TDirectory* rsub = Utils::makeDir(sub, region.name);
      hists.regions.push_back(makeClusterHists(sensor, region.areaPixel, rsub));
    }

    m_hists.push_back(std::move(hists));
  }
}

std::string Analyzers::ClusterInfo::name() const { return "ClusterInfo"; }

void Analyzers::ClusterInfo::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < m_hists.size(); ++isensor) {
    SensorHists& hists = m_hists[isensor];
    const Storage::Plane& plane = *event.getPlane(isensor);

    hists.nClusters->Fill(plane.numClusters());
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *plane.getCluster(icluster);
      hists.fill(cluster);
    }
  }
}

void Analyzers::ClusterInfo::finalize()
{
  auto fillRate = [](const TH2D* map, const double numEvents, TH1D* rate) {
    // rescale rate histogram to available range
    rate->SetBins(rate->GetNbinsX(), 0, map->GetMaximum() / numEvents);
    rate->Reset();
    // fill rate
    for (int ix = 1; ix <= map->GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= map->GetNbinsY(); ++iy) {
        auto count = map->GetBinContent(ix, iy);
        if (count != 0)
          rate->Fill(count / numEvents);
      }
    }
  };

  for (auto& hists : m_hists)
    fillRate(hists.whole.pos, hists.nClusters->GetEntries(), hists.rate);
}
