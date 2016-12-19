#include "clusterinfo.h"

#include <cassert>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::ClusterInfo::ClusterInfo(const Mechanics::Device* device,
                                    TDirectory* dir,
                                    const char* suffix,
                                    const int sizeMax,
                                    const int timeMax,
                                    const int valueMax,
                                    const int binsUncertainty)
    : SingleAnalyzer(device, dir, suffix, "ClusterInfo")
{
  using namespace Utils;

  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = makeDir(dir, "ClusterInfo");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);
    auto name = [&](std::string suffix) {
      return sensor.name() + '-' + suffix;
    };

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
    hs.size = makeH1(sub, name("Size"), axSize);
    hs.sizeSizeCol = makeH2(sub, name("SizeCol_Size"), axSize, axSizeCol);
    hs.sizeSizeRow = makeH2(sub, name("SizeRow_Size"), axSize, axSizeRow);
    hs.sizeColSizeRow =
        makeH2(sub, name("SizeRow_SizeCol"), axSizeCol, axSizeRow);
    hs.value = makeH1(sub, name("Value"), axValue);
    hs.sizeValue = makeH2(sub, name("Value_Size"), axSize, axValue);
    hs.uncertaintyU = makeH1(sub, name("UncertaintyU"), axUnU);
    hs.uncertaintyV = makeH1(sub, name("UncertaintyV"), axUnV);
    hs.sizeHitTime = makeH2(sub, name("HitTime_Size"), axSize, axHitTime);
    hs.sizeHitValue = makeH2(sub, name("HitValue_Size"), axSize, axHitValue);
    hs.hitValueHitTime =
        makeH2(sub, name("HitTime_HitValue"), axValue, axHitTime);
    m_hists.push_back(hs);
  }
}

void Analyzers::ClusterInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (Index iplane = 0; iplane < event->numPlanes(); ++iplane) {
    const Storage::Plane& plane = *event->getPlane(iplane);
    ClusterHists& hs = m_hists.at(iplane);

    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *plane.getCluster(icluster);

      // Check if the cluster passes the cuts
      if (!checkCuts(&cluster))
        continue;

      hs.size->Fill(cluster.size());
      hs.sizeSizeCol->Fill(cluster.size(), cluster.sizeCol());
      hs.sizeSizeRow->Fill(cluster.size(), cluster.sizeRow());
      hs.sizeColSizeRow->Fill(cluster.sizeCol(), cluster.sizeRow());
      hs.value->Fill(cluster.value());
      hs.sizeValue->Fill(cluster.size(), cluster.value());
      Vector2 stddev = sqrt(cluster.covLocal().Diagonal());
      hs.uncertaintyU->Fill(stddev[0]);
      hs.uncertaintyV->Fill(stddev[1]);
      for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
        const Storage::Hit& hit = *cluster.getHit(ihit);
        hs.sizeHitTime->Fill(cluster.size(), hit.time());
        hs.sizeHitValue->Fill(cluster.size(), hit.value());
        hs.hitValueHitTime->Fill(hit.value(), hit.time());
      }
    }
  }
}

void Analyzers::ClusterInfo::postProcessing() {}
