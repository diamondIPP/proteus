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

using namespace Storage;

// return true if both hits are connected, i.e. share one edge
//
// WARNING: hits w/ the same position are counted as connected
static inline bool connected(const Hit& hit0, const Hit& hit1)
{
  auto dc = std::abs(hit1.col() - hit0.col());
  auto dr = std::abs(hit1.row() - hit0.row());
  return (hit0.region() == hit1.region()) and
         (((dc == 0) and (dr <= 1)) or ((dc <= 1) and (dr == 0)));
}

// return true if the hit is connected to any hit in the range
template <typename HitIterator>
static inline bool
connected(HitIterator clusterBegin, HitIterator clusterEnd, const Hit& hit)
{
  bool flag = false;
  for (; clusterBegin != clusterEnd; ++clusterBegin) {
    flag = (flag or connected(*clusterBegin->get(), hit));
  }
  return flag;
}

// rearange the input hit range so that pixels in a cluster are neighbours.
template <typename HitIterator>
static inline void inplaceCluster(HitIterator hitsBegin,
                                  HitIterator hitsEnd,
                                  Storage::SensorEvent& sensorEvent)
{
  auto clusterBegin = hitsBegin;
  while (clusterBegin != hitsEnd) {
    // every cluster has at least one member
    auto clusterEnd = std::next(clusterBegin);

    // each iteration can only pick up the nearest-neighboring pixels, so we
    // need to iterate until we find no more connected pixels.
    while (clusterEnd != hitsEnd) {
      // accumulate all connected hits to the beginning of the range
      auto moreHits = std::partition(clusterEnd, hitsEnd, [=](const auto& hit) {
        return connected(clusterBegin, clusterEnd, *hit);
      });
      // no connected hits were found -> cluster is complete
      if (moreHits == clusterEnd) {
        break;
      }
      // some connected hits were found -> extend cluster
      clusterEnd = moreHits;
    }

    // sort cluster hits by value and time
    std::sort(clusterBegin, clusterEnd, [](const auto& hit0, const auto& hit1) {
      return (hit1->value() < hit0->value()) or
             (hit0->timestamp() < hit1->timestamp());
    });
    // add cluster to event
    auto& cluster = sensorEvent.addCluster();
    for (auto hit = clusterBegin; hit != clusterEnd; ++hit) {
      cluster.addHit(*hit->get());
    }

    // only consider the remaining hits for the next cluster
    clusterBegin = clusterEnd;
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

  // move masked pixels to the back of the hit vector
  auto hitsEnd = std::partition(sensorEvent.m_hits.begin(),
                                sensorEvent.m_hits.end(), [&](const auto& hit) {
                                  return not m_sensor.pixelMask().isMasked(
                                      hit->col(), hit->row());
                                });
  // group pixels into clusters by rearanging the hit vector
  inplaceCluster(sensorEvent.m_hits.begin(), hitsEnd, sensorEvent);
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
