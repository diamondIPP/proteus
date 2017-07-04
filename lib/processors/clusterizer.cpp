/*
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "clusterizer.h"

#include <algorithm>
#include <functional>
#include <limits>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

// return true if both hits are connected, i.e. share one edge
//
// WARNING: hits w/ the same position are counted as connected
static bool connected(const Storage::Hit& hit0, const Storage::Hit& hit1)
{
  auto dc = std::abs(hit1.col() - hit0.col());
  auto dr = std::abs(hit1.row() - hit0.row());
  bool sameRegion = (hit0.region() == hit1.region());
  return sameRegion && (((dc == 0) && (dr <= 1)) || ((dc <= 1) && (dr == 0)));
}

static void cluster(std::vector<std::reference_wrapper<Storage::Hit>>& hits,
                    Storage::SensorEvent& sensorEvent)
{
  while (!hits.empty()) {
    auto clusterStart = --hits.end();
    auto clusterEnd = hits.end();

    // accumulate all connected hits at the end of the vector until
    // no more compatible hits can be found.
    // each iteration can only pick up the next neighboring pixels, so we
    // need to iterate until we find no more connected pixels.
    while (true) {
      auto unconnected = [&](const Storage::Hit& hit) {
        using namespace std::placeholders;
        return std::none_of(clusterStart, clusterEnd,
                            std::bind(connected, hit, _1));
      };
      // connected hits end up in [moreHits, clusterStart)
      auto moreHits = std::partition(hits.begin(), clusterStart, unconnected);
      if (moreHits == clusterStart)
        break;
      clusterStart = moreHits;
    }

    // construct cluster from connected hits
    Storage::Cluster& cluster = sensorEvent.addCluster();
    for (auto hit = clusterStart; hit != clusterEnd; ++hit)
      cluster.addHit(*hit);

    // remove clustered hits from further consideration
    hits.erase(clusterStart, clusterEnd);
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
  auto& sensorEvent = event.getSensorEvent(m_sensor.id());
  std::vector<std::reference_wrapper<Storage::Hit>> hits;

  // do not cluster masked pixels
  hits.clear();
  hits.reserve(sensorEvent.numHits());
  for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
    Storage::Hit& hit = sensorEvent.getHit(ihit);
    if (m_sensor.pixelMask().isMasked(hit.col(), hit.row()))
      continue;
    hits.push_back(std::ref(hit));
  }
  cluster(hits, sensorEvent);

  // estimate cluster properties
  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster)
    estimateProperties(sensorEvent.getCluster(icluster));
}

Processors::BinaryClusterizer::BinaryClusterizer(
    const Mechanics::Sensor& sensor)
    : BaseClusterizer("BinaryClusterizer", sensor)
{
  DEBUG("binary clustering for ", sensor.name());
}

static const Vector3 HIT_COV_ENTRIES(1. / 12., 0., 1. / 12.);
// set upper triangular part of the covariance matrix
static const SymMatrix2 HIT_COV(HIT_COV_ENTRIES, false);

void Processors::BinaryClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  XYPoint pos(0, 0);
  float time = std::numeric_limits<float>::max();

  for (const Storage::Hit& hit : cluster.hits()) {
    pos += XYVector(hit.posPixel());
    time = std::min(time, hit.time());
  }
  pos /= cluster.size();

  // 1/12 factor from pixel size to stddev of equivalent gaussian
  SymMatrix2 cov;
  cov(0, 0) = 1.0 / (12.0f * cluster.sizeCol());
  cov(0, 1) = 0;
  cov(1, 0) = 0;
  cov(1, 1) = 1.0 / (12.0f * cluster.sizeRow());

  cluster.setPixel(pos, cov);
  cluster.setTime(time);
  cluster.setValue(cluster.size());
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
  float time = std::numeric_limits<float>::max();
  float value = 0;

  for (const Storage::Hit& hit : cluster.hits()) {
    pos += hit.value() * XYVector(hit.posPixel());
    time = std::min(time, hit.time());
    value += hit.value();
  }
  pos /= value;

  // TODO 2016-11-14 msmk: consider also the value weighting
  // 1/12 factor from pixel size to stddev of equivalent gaussian
  SymMatrix2 cov;
  cov(0, 0) = 1.0 / (12.0f * cluster.sizeCol());
  cov(0, 1) = 0;
  cov(1, 0) = 0;
  cov(1, 1) = 1.0 / (12.0f * cluster.sizeRow());

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
  float time = std::numeric_limits<float>::max();
  float value = 0;

  for (const Storage::Hit& hit : cluster.hits()) {
    if (hit.time() < time) {
      pos = hit.posPixel();
      time = hit.time();
      value = hit.value();
    }
  }

  // 1/12 factor from pixel size to stddev of equivalent gaussian
  SymMatrix2 cov;
  cov(0, 0) = 1.0 / 12.0;
  cov(0, 1) = 0;
  cov(1, 0) = 0;
  cov(1, 1) = 1.0 / 12.0;

  cluster.setPixel(pos, cov);
  cluster.setTime(time);
  cluster.setValue(value);
}
