#include "clusterinfo.h"

#include <cassert>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"

Analyzers::ClusterInfo::ClusterInfo(const Mechanics::Device* device,
                                    TDirectory* dir,
                                    const char* suffix,
                                    const int sizeMax,
                                    const int timeMax,
                                    const int valueMax,
                                    const int binsUncertainty)
    : SingleAnalyzer(device, dir, suffix, "ClusterInfo")
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("ClusterInfo");

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);

    auto h1 = [&](std::string name, double x0, double x1,
                  int nx = -1) -> TH1D* {
      if (nx < 0)
        nx = std::round(x1 - x0);
      TH1D* h = new TH1D((sensor.name() + '-' + name).c_str(), "", nx, x0, x1);
      h->SetDirectory(plotDir);
      return h;
    };
    auto h2 = [&](std::string name, double x0, double x1, double y0, double y1,
                  int nx = -1, int ny = -1) -> TH2D* {
      if (nx < 0)
        nx = std::round(x1 - x0);
      if (ny < 0)
        ny = std::round(y1 - y0);
      TH2D* h = new TH2D((sensor.name() + '-' + name).c_str(), "", nx, x0, x1,
                         ny, y0, y1);
      h->SetDirectory(plotDir);
      return h;
    };

    ClusterHists hs;
    hs.size = h1("Size", 0.5, sizeMax - 0.5);
    hs.sizeSizeCol = h2("SizeCol_Size", 0.5, sizeMax - 0.5, 0.5, sizeMax - 0.5);
    hs.sizeSizeRow = h2("SizeRow_Size", 0.5, sizeMax - 0.5, 0.5, sizeMax - 0.5);
    hs.sizeColSizeRow =
        h2("SizeCol_SizeRow", 0.5, sizeMax - 0.5, 0.5, sizeMax - 0.5);
    hs.value = h1("Value", -0.5, valueMax - 0.5);
    hs.sizeValue = h2("Value_Size", 0.5, sizeMax - 0.5, -0.5, valueMax - 0.5);
    hs.uncertaintyU =
        h1("UncertaintyU", 0, sensor.pitchCol() / 2, binsUncertainty);
    hs.uncertaintyV =
        h1("UncertaintyV", 0, sensor.pitchRow() / 2, binsUncertainty);
    hs.sizeHitTime =
        h2("HitTime_Size", 0.5, sizeMax - 0.5, -0.5, timeMax - 0.5);
    hs.sizeHitValue =
        h2("HitValue_Size", 0.5, sizeMax - 0.5, -0.5, valueMax - 0.5);
    hs.hitValueHitTime =
        h2("HitTime_HitValue", -0.5, valueMax - 0.5, -0.5, timeMax - 0.5);
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
