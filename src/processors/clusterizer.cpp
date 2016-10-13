/*
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "clusterizer.h"

#include <algorithm>
#include <limits>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/eventloop.h"

/** Check if the given hit is unconnected to the cluster. */
struct Unconnected {
  const Storage::Cluster* cluster;
  double maxDistSquared;

  bool operator()(const Storage::Hit* hit)
  {
    for (Index icompare = 0; icompare < cluster->numHits(); ++icompare) {
      XYVector delta = hit->posPixel() - cluster->getHit(icompare)->posPixel();
      if (delta.Mag2() <= maxDistSquared)
        return false;
    }
    return true;
  }
};

static void cluster(double maxDistSquared,
                    std::vector<Storage::Hit*>& hits,
                    Storage::Plane& plane)
{
  while (!hits.empty()) {
    Storage::Hit* seed = hits.back();
    hits.pop_back();

    Storage::Cluster* cluster = plane.newCluster();
    cluster->addHit(seed);

    while (!hits.empty()) {
      // all hits connected to the cluster are moved to the end
      auto connected = std::partition(
          hits.begin(), hits.end(), Unconnected{cluster, maxDistSquared});
      // all other hits are unconnected, cluster is complete
      if (connected == hits.end())
        break;
      // add connected hits to cluster and remove from further consideration
      for (auto hit = connected; hit != hits.end(); ++hit) {
        cluster->addHit(*hit);
      }
      hits.erase(connected, hits.end());
    }
  }
}

std::string Processors::BaseClusterizer::name() const { return m_name; }

void Processors::BaseClusterizer::process(Storage::Event& event) const
{
  std::vector<Storage::Hit*> hits;

  for (auto id = m_sensorIds.begin(); id != m_sensorIds.end(); ++id) {
    const Mechanics::Sensor& sensor = *m_device.getSensor(*id);
    Storage::Plane& plane = *event.getPlane(*id);

    // don't try to cluster noise hits
    hits.clear();
    hits.reserve(plane.numHits());
    for (Index ihit = 0; ihit < plane.numHits(); ++ihit) {
      Storage::Hit* hit = plane.getHit(ihit);
      if (sensor.isPixelNoisy(hit->col(), hit->row()))
        continue;
      hits.push_back(hit);
    }
    cluster(m_maxDistSquared, hits, plane);
    
    // estimate cluster properties
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster)
      estimateProperties(*plane.getCluster(icluster));
  }
}

void Processors::BinaryClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos;
  double time = std::numeric_limits<double>::max();
  double scale = 1 / cluster.numHits();

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    pos += scale * XYVector(hit->posPixel());
    time = std::min(time, hit->timing());
  }

  // TODO 2016-10-13 msmk: correctly estimate errors

  cluster.setPosPixel(pos);
  // 1 / sqrt(12) factor from pixel size to stddev of equivalent gaussian
  cluster.setErrPixel(XYVector(0.288675134594813, 0.288675134594813));
  cluster.setTiming(time);
}

void Processors::ValueWeightedClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos;
  double time = std::numeric_limits<double>::max();
  double weight = 0;

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    pos += hit->value() * XYVector(hit->posPixel());
    time = std::min(time, hit->timing());
    weight += hit->value();
  }
  pos /= weight;

  // TODO 2016-10-13 msmk: correctly estimate errors

  cluster.setPosPixel(pos);
  // 1 / sqrt(12) factor from pixel size to stddev of equivalent gaussian
  cluster.setErrPixel(XYVector(0.288675134594813, 0.288675134594813));
  cluster.setTiming(time);
}

void Processors::FastestHitClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos;
  double time = std::numeric_limits<double>::max();

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    if (hit->timing() < time) {
      pos = hit->posPixel();
      time = hit->timing();
    }
  }

  cluster.setPosPixel(pos);
  // 1 / sqrt(12) factor from pixel size to stddev of equivalent gaussian
  cluster.setErrPixel(XYVector(0.288675134594813, 0.288675134594813));
  cluster.setTiming(time);
}

void Processors::setupClusterizer(const Mechanics::Device& device,
                                  Utils::EventLoop& loop)
{
  using Mechanics::Sensor;

  std::set<Index> binary;
  std::set<Index> weighted;

  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    const Mechanics::Sensor* sensor = device.getSensor(isensor);
    switch (sensor->measurement()) {
    case Sensor::PIXEL_BINARY:
    case Sensor::CCPDV4_BINARY:
      binary.insert(isensor);
      break;
    case Sensor::PIXEL_TOT:
      weighted.insert(isensor);
    }
  }

  if (!binary.empty())
    loop.addProcessor(std::make_shared<BinaryClusterizer>(device, binary));
  if (!weighted.empty())
    loop.addProcessor(
        std::make_shared<ValueWeightedClusterizer>(device, weighted));
}
