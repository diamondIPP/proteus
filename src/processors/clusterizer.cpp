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
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

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
      auto connected = std::partition(hits.begin(), hits.end(),
                                      Unconnected{cluster, maxDistSquared});
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

Processors::BaseClusterizer::BaseClusterizer(const std::string& namePrefix,
                                             const Mechanics::Device& device,
                                             Index sensorId)
    : m_sensor(*device.getSensor(sensorId))
    , m_maxDistSquared(1)
    , m_name(namePrefix + "(sensorId=" + std::to_string(sensorId) + ')')
{
}

std::string Processors::BaseClusterizer::name() const { return m_name; }

void Processors::BaseClusterizer::process(Storage::Event& event) const
{
  std::vector<Storage::Hit*> hits;
  Storage::Plane& plane = *event.getPlane(m_sensor.id());

  // don't try to cluster noise hits
  hits.clear();
  hits.reserve(plane.numHits());
  for (Index ihit = 0; ihit < plane.numHits(); ++ihit) {
    Storage::Hit* hit = plane.getHit(ihit);
    if (m_sensor.isPixelNoisy(hit->col(), hit->row()))
      continue;
    hits.push_back(hit);
  }
  cluster(m_maxDistSquared, hits, plane);

  // estimate cluster properties
  for (Index icluster = 0; icluster < plane.numClusters(); ++icluster)
    estimateProperties(*plane.getCluster(icluster));
}

Processors::BinaryClusterizer::BinaryClusterizer(
    const Mechanics::Device& device, Index sensorId)
    : BaseClusterizer("BinaryClusterizer", device, sensorId)
{
  DEBUG("binary clustering for sensor ", sensorId);
}

// 1/12 factor from pixel size to stddev of equivalent gaussian
static const Vector3 HIT_COV_ENTRIES(1. / 12., 0., 1. / 12.);
// set upper triangular part of the covariance matrix
static const SymMatrix2 HIT_COV(HIT_COV_ENTRIES, false);

void Processors::BinaryClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos;
  double time = std::numeric_limits<double>::max();

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    pos += XYVector(hit->posPixel());
    time = std::min(time, hit->time());
  }
  pos /= cluster.numHits();

  SymMatrix2 cov = HIT_COV;
  cov(0, 0) /= cluster.sizeCol();
  cov(1, 1) /= cluster.sizeRow();

  cluster.setPixel(pos, cov);
  cluster.setTime(time);
}

Processors::ValueWeightedClusterizer::ValueWeightedClusterizer(
    const Mechanics::Device& device, Index sensorId)
    : BaseClusterizer("ValueWeightedClusterizer", device, sensorId)
{
  DEBUG("value weighted clustering for sensor ", sensorId);
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
    time = std::min(time, hit->time());
    weight += hit->value();
  }
  pos /= weight;

  // TODO 2016-11-14 msmk: consider also the value weighting
  SymMatrix2 cov = HIT_COV;
  cov(0, 0) /= cluster.sizeCol();
  cov(1, 1) /= cluster.sizeRow();

  cluster.setPixel(pos, cov);
  cluster.setTime(time);
}

Processors::FastestHitClusterizer::FastestHitClusterizer(
    const Mechanics::Device& device, Index sensorId)
    : BaseClusterizer("FastestHitClusterizer", device, sensorId)
{
  DEBUG("fastest hit (non-)clustering for sensor ", sensorId);
}

void Processors::FastestHitClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos;
  double time = std::numeric_limits<double>::max();

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    if (hit->time() < time) {
      pos = hit->posPixel();
      time = hit->time();
    }
  }

  cluster.setPixel(pos, HIT_COV);
  cluster.setTime(time);
}

void Processors::setupClusterizers(const Mechanics::Device& device,
                                   Utils::EventLoop& loop)
{
  for (Index sensorId = 0; sensorId < device.numSensors(); ++sensorId) {
    const Mechanics::Sensor* sensor = device.getSensor(sensorId);
    switch (sensor->measurement()) {
    case Mechanics::Sensor::Measurement::PixelBinary:
    case Mechanics::Sensor::Measurement::Ccpdv4Binary:
      loop.addProcessor(std::make_shared<BinaryClusterizer>(device, sensorId));
      break;
    case Mechanics::Sensor::Measurement::PixelTot:
      loop.addProcessor(
          std::make_shared<ValueWeightedClusterizer>(device, sensorId));
      break;
    }
  }
}
