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

// return true if both hits are connected, i.e. share one edge.
//
// WARNING: hits w/ the same position are counted as connected
static bool connected(const Storage::Hit* hit0, const Storage::Hit* hit1)
{
  auto dc = std::abs(int(hit1->col()) - int(hit0->col()));
  auto dr = std::abs(int(hit1->row()) - int(hit0->row()));
  return ((dc == 0) && (dr <= 1)) || ((dc <= 1) && (dr == 0));
}

// return true if the hit is connected to any hit in the cluster.
static bool connected(const Storage::Cluster* cluster, const Storage::Hit* hit)
{
  for (Index ihit = 0; ihit < cluster->numHits(); ++ihit) {
    if (connected(cluster->getHit(ihit), hit))
      return true;
  }
  return false;
}

// return true if the hit and cluster are connected and in the same region
static bool compatible(const Storage::Cluster* cluster, const Storage::Hit* hit)
{
  return (cluster->region() == hit->region()) && connected(cluster, hit);
}

static void cluster(std::vector<Storage::Hit*>& hits, Storage::Plane& plane)
{
  while (!hits.empty()) {
    Storage::Hit* seed = hits.back();
    hits.pop_back();

    Storage::Cluster* cluster = plane.newCluster();
    cluster->addHit(seed);

    while (!hits.empty()) {
      // move all compatible hits to the end
      auto connected = std::partition(
          hits.begin(), hits.end(),
          [&](const Storage::Hit* hit) { return !compatible(cluster, hit); });
      // there are no more compatible hits and the cluster is complete
      if (connected == hits.end())
        break;
      // add compatible hits to cluster and remove from further consideration
      for (auto hit = connected; hit != hits.end(); ++hit)
        cluster->addHit(*hit);
      hits.erase(connected, hits.end());
    }
  }
}

Processors::BaseClusterizer::BaseClusterizer(const std::string& namePrefix,
                                             const Mechanics::Sensor& sensor)
    : m_sensor(sensor), m_name(namePrefix + '(' + sensor.name() + ')')
{
}

std::string Processors::BaseClusterizer::name() const { return m_name; }

void Processors::BaseClusterizer::process(Storage::Event& event) const
{
  std::vector<Storage::Hit*> hits;
  Storage::Plane& plane = *event.getPlane(m_sensor.id());

  // don't try to cluster masked pixels
  hits.clear();
  hits.reserve(plane.numHits());
  for (Index ihit = 0; ihit < plane.numHits(); ++ihit) {
    Storage::Hit* hit = plane.getHit(ihit);
    if (m_sensor.pixelMask().isMasked(hit->col(), hit->row()))
      continue;
    hits.push_back(hit);
  }
  cluster(hits, plane);

  // estimate cluster properties
  for (Index icluster = 0; icluster < plane.numClusters(); ++icluster)
    estimateProperties(*plane.getCluster(icluster));
}

Processors::BinaryClusterizer::BinaryClusterizer(
    const Mechanics::Sensor& sensor)
    : BaseClusterizer("BinaryClusterizer", sensor)
{
  DEBUG("binary clustering for ", sensor.name());
}

// 1/12 factor from pixel size to stddev of equivalent gaussian
static const Vector3 HIT_COV_ENTRIES(1. / 12., 0., 1. / 12.);
// set upper triangular part of the covariance matrix
static const SymMatrix2 HIT_COV(HIT_COV_ENTRIES, false);

void Processors::BinaryClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos(0, 0);
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
  cluster.setValue(cluster.numHits());
}

Processors::ValueWeightedClusterizer::ValueWeightedClusterizer(
    const Mechanics::Sensor& sensor)
    : BaseClusterizer("ValueWeightedClusterizer", sensor)
{
  DEBUG("value weighted clustering for ", sensor.name());
}

void Processors::ValueWeightedClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos(0, 0);
  double time = std::numeric_limits<double>::max();
  double value = 0;

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    pos += hit->value() * XYVector(hit->posPixel());
    time = std::min(time, hit->time());
    value += hit->value();
  }
  pos /= value;

  // TODO 2016-11-14 msmk: consider also the value weighting
  SymMatrix2 cov = HIT_COV;
  cov(0, 0) /= cluster.sizeCol();
  cov(1, 1) /= cluster.sizeRow();

  cluster.setPixel(pos, cov);
  cluster.setTime(time);
  cluster.setValue(value);
}

Processors::FastestHitClusterizer::FastestHitClusterizer(
    const Mechanics::Sensor& sensor)
    : BaseClusterizer("FastestHitClusterizer", sensor)
{
  DEBUG("fastest hit (non-)clustering for ", sensor.name());
}

void Processors::FastestHitClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos(0, 0);
  double time = std::numeric_limits<double>::max();
  double value = -1;

  for (Index ihit = 0; ihit < cluster.numHits(); ++ihit) {
    const Storage::Hit* hit = cluster.getHit(ihit);
    if (hit->time() < time) {
      pos = hit->posPixel();
      time = hit->time();
      value = hit->value();
    }
  }

  cluster.setPixel(pos, HIT_COV);
  cluster.setTime(time);
  cluster.setValue(value);
}
