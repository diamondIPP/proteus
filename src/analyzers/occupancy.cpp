#include "occupancy.h"

#include <cassert>
#include <iostream>
#include <math.h>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Occupancy::Hists::Hists(const Mechanics::Sensor& sensor,
                                   TDirectory* dir,
                                   const int occupancyBins)
{
  using namespace Utils;

  auto area = sensor.sensitiveAreaPixel();
  auto name = [&](const std::string& suffix) {
    return sensor.name() + '-' + suffix;
  };

  HistAxis axHitCol(area.interval(0), area.length(0), "Hit column");
  HistAxis axHitRow(area.interval(1), area.length(1), "Hit row");
  HistAxis axClusterCol(area.interval(0), area.length(0), "Cluster column");
  HistAxis axClusterRow(area.interval(1), area.length(1), "Cluster row");
  HistAxis axHitDist(0, 1, occupancyBins, "Hits / pixel / event");
  HistAxis axClusterDist(0, 1, occupancyBins, "Clusters / pixel / event");

  hitMap = makeH2(dir, name("HitMap"), axHitCol, axHitRow);
  hitOccupancyDist = makeH1(dir, name("HitOccupancyDist"), axHitDist);
  clusteredHitMap = makeH2(dir, name("ClusteredHitMap"), axHitCol, axHitRow);
  clusteredHitOccupancyDist =
      makeH1(dir, name("ClusteredHitOccupancyDist"), axHitDist);
  clusterMap = makeH2(dir, name("ClusterMap"), axClusterCol, axClusterRow);
  clusterOccupancyDist =
      makeH1(dir, name("ClusterOccupancyDist"), axClusterDist);
}

Analyzers::Occupancy::Occupancy(const Mechanics::Device* device,
                                TDirectory* dir)
{
  assert(device && "Analyzer: can't initialize with null device");

  TDirectory* sub = Utils::makeDir(dir, "Occupancy");
  for (Index isensor = 0; isensor < device->numSensors(); isensor++) {
    m_hists.emplace_back(*device->getSensor(isensor), sub);
  }
}

std::string Analyzers::Occupancy::name() const { return "Occupancy"; }

TH2D* Analyzers::Occupancy::getHitOcc(Index isensor)
{
  return m_hists.at(isensor).hitMap;
}

TH1D* Analyzers::Occupancy::getHitOccDist(Index isensor)
{
  return m_hists.at(isensor).hitOccupancyDist;
}

uint64_t Analyzers::Occupancy::getTotalHitOccupancy(Index isensor)
{
  return static_cast<uint64_t>(getHitOcc(isensor)->GetEntries());
}

void Analyzers::Occupancy::analyze(const Storage::Event& event)
{
  m_numEvents += 1;

  for (Index isensor = 0; isensor < m_hists.size(); ++isensor) {
    Hists& hists = m_hists[isensor];
    const Storage::Plane& sensorEvent = *event.getPlane(isensor);

    for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
      const Storage::Hit& hit = *sensorEvent.getHit(ihit);

      hists.hitMap->Fill(hit.posPixel().x(), hit.posPixel().y());
    }

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *sensorEvent.getCluster(icluster);

      hists.clusterMap->Fill(cluster.posPixel().x(), cluster.posPixel().y());

      for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
        const Storage::Hit& hit = *cluster.getHit(ihit);

        hists.clusteredHitMap->Fill(hit.posPixel().x(), hit.posPixel().y());
      }
    }
  }
}

void Analyzers::Occupancy::finalize()
{
  auto fillOccupancyDist = [&](const TH2D* map, TH1D* dist) {
    // rescale occupancy histogram to available range
    dist->SetBins(dist->GetNbinsX(), 0, map->GetMaximum() / m_numEvents);
    dist->Reset();
    // fill occupancy
    for (int ix = 1; ix <= map->GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= map->GetNbinsY(); ++iy) {
        auto count = map->GetBinContent(ix, iy);
        if (count != 0)
          dist->Fill(count / m_numEvents);
      }
    }
  };

  for (auto& hists : m_hists) {
    fillOccupancyDist(hists.hitMap, hists.hitOccupancyDist);
    fillOccupancyDist(hists.clusteredHitMap, hists.clusteredHitOccupancyDist);
    fillOccupancyDist(hists.clusterMap, hists.clusterOccupancyDist);
  }
}
