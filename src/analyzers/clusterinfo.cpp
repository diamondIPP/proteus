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
  size->Fill(cluster.size());
  sizeSizeCol->Fill(cluster.size(), cluster.sizeCol());
  sizeSizeRow->Fill(cluster.size(), cluster.sizeRow());
  sizeColSizeRow->Fill(cluster.sizeCol(), cluster.sizeRow());
  value->Fill(cluster.value());
  sizeValue->Fill(cluster.size(), cluster.value());
  uncertaintyU->Fill(std::sqrt(cluster.covLocal()(0, 0)));
  uncertaintyV->Fill(std::sqrt(cluster.covLocal()(1, 1)));
  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit& hit = *cluster.getHit(ihit);
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

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = makeDir(dir, "ClusterInfo");

  auto makeClusterHists = [&](const Mechanics::Sensor& sensor,
                              std::string suffix) {
    std::string prefix = sensor.name() + '-';
    if (!suffix.empty()) {
      prefix += suffix;
      prefix += '-';
    }

    HistAxis axSize(1, sizeMax, "Cluster size");
    HistAxis axSizeCol(1, sizeMax, "Cluster column size");
    HistAxis axSizeRow(1, sizeMax, "Cluster row size");
    HistAxis axValue(0, valueMax, "Cluster value");
    HistAxis axHitTime(0, timeMax, "Hit time");
    HistAxis axHitValue(0, valueMax, "Hit value");
    HistAxis axUnU(0, sensor.pitchCol() / 2, binsUncertainty,
                   "Cluster uncertainty u");
    HistAxis axUnV(0, sensor.pitchRow() / 2, binsUncertainty,
                   "Cluster uncertainty v");

    ClusterHists hs;
    hs.size = makeH1(sub, prefix + "Size", axSize);
    hs.sizeSizeCol = makeH2(sub, prefix + "SizeCol_Size", axSize, axSizeCol);
    hs.sizeSizeRow = makeH2(sub, prefix + "SizeRow_Size", axSize, axSizeRow);
    hs.sizeColSizeRow =
        makeH2(sub, prefix + "SizeRow_SizeCol", axSizeCol, axSizeRow);
    hs.value = makeH1(sub, prefix + "Value", axValue);
    hs.sizeValue = makeH2(sub, prefix + "Value_Size", axSize, axValue);
    hs.uncertaintyU = makeH1(sub, prefix + "UncertaintyU", axUnU);
    hs.uncertaintyV = makeH1(sub, prefix + "UncertaintyV", axUnV);
    hs.sizeHitTime = makeH2(sub, prefix + "HitTime_Size", axSize, axHitTime);
    hs.sizeHitValue = makeH2(sub, prefix + "HitValue_Size", axSize, axHitValue);
    hs.hitValueHitTime =
        makeH2(sub, prefix + "HitTime_HitValue", axHitValue, axHitTime);
    return hs;
  };

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);

    SensorHists hists;
    hists.whole = makeClusterHists(sensor, std::string());
    for (const auto& region : sensor.regions())
      hists.regions.push_back(makeClusterHists(sensor, region.name));

    m_hists.push_back(std::move(hists));
  }
}

std::string Analyzers::ClusterInfo::name() const { return "ClusterInfo"; }

void Analyzers::ClusterInfo::analyze(const Storage::Event& event)
{
  for (Index isensor = 0; isensor < m_hists.size(); ++isensor) {
    SensorHists& hists = m_hists[isensor];

    const Storage::Plane& plane = *event.getPlane(isensor);
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *plane.getCluster(icluster);

      hists.fill(cluster);
    }
  }
}

void Analyzers::ClusterInfo::finalize() {}
