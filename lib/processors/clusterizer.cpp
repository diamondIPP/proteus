/*
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "clusterizer.h"

#include <algorithm>
#include <functional>
#include <limits>

#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "storage/event.h"
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
      if (moreHits == clusterStart) {
        break;
      }
      clusterStart = moreHits;
    }

    // construct cluster from connected hits
    Storage::Cluster& cluster = sensorEvent.addCluster();
    for (auto hit = clusterStart; hit != clusterEnd; ++hit) {
      cluster.addHit(*hit);
    }

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

void Processors::BaseClusterizer::execute(Storage::Event& event) const
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
  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    estimateProperties(sensorEvent.getCluster(icluster));
  }
}

Processors::BinaryClusterizer::BinaryClusterizer(
    const Mechanics::Sensor& sensor)
    : BaseClusterizer("BinaryClusterizer", sensor)
{
  DEBUG("binary clustering for ", sensor.name());
}

void Processors::BinaryClusterizer::estimateProperties(
    Storage::Cluster& cluster) const
{
  Vector2 pos = Vector2::Zero();
  int ts = std::numeric_limits<int>::max();
  int value = 0;

  for (const Storage::Hit& hit : cluster.hits()) {
    pos += Vector2(hit.col(), hit.row());
    ts = std::min(ts, hit.timestamp());
    value += hit.value();
  }
  pos /= cluster.size();

  // 1/12 factor from pixel size to stdev of equivalent gaussian
  Vector2 var(1.0f / (12.0f * cluster.sizeCol()),
              1.0f / (12.0f * cluster.sizeRow()));
  cluster.setPixel(pos, var.asDiagonal(), ts);
  cluster.setValue(value);
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
  Vector2 pos = Vector2::Zero();
  int ts = std::numeric_limits<int>::max();
  int value = 0;

  for (const Storage::Hit& hit : cluster.hits()) {
    pos += hit.value() * Vector2(hit.col(), hit.row());
    ts = std::min(ts, hit.timestamp());
    value += hit.value();
  }
  pos /= value;

  // TODO 2016-11-14 msmk: consider also the value weighting
  // 1/12 factor from pixel size to stdev of equivalent gaussian
  Vector2 var(1.0f / (12.0f * cluster.sizeCol()),
              1.0f / (12.0f * cluster.sizeRow()));
  cluster.setPixel(pos, var.asDiagonal(), ts);
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
  Vector2 pos = Vector2::Zero();
  int ts = std::numeric_limits<int>::max();
  int value = 0;

  for (const Storage::Hit& hit : cluster.hits()) {
    if (hit.timestamp() < ts) {
      pos = Vector2(hit.col(), hit.row());
      ts = hit.timestamp();
      value = hit.value();
    }
  }

  // 1/12 factor from pixel size to stdev of equivalent gaussian
  cluster.setPixel(pos, Vector2::Constant(1.0 / 12.0f).asDiagonal(), ts);
  cluster.setValue(value);
}
