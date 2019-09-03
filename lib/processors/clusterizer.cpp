// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
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
#include "storage/sensorevent.h"
#include "utils/interval.h"
#include "utils/logger.h"

namespace proteus {

using DigitalRange = Interval<int>;

// scaling from uniform with to equivalent Gaussian standard deviation
constexpr Scalar kVar = 1.0 / 12.0;

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
template <typename HitIterator, typename ClusterMaker>
static inline void clusterize(const DenseMask& mask,
                              SensorEvent& sensorEvent,
                              HitIterator hitsBegin,
                              HitIterator hitsEnd,
                              ClusterMaker makeCluster)
{
  // move masked pixels to the back of the hit vector
  hitsEnd = std::partition(hitsBegin, hitsEnd, [&](const auto& hit) {
    return not mask.isMasked(hit->col(), hit->row());
  });

  // group all connected hits starting from an arbitrary seed hit (first hit).
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
    // WARNING compare has to fullfil (from C++ standard)
    //   1. compare(a, a) == false
    //   2. compare(a, b) == true -> compare(b, a) == false
    //   3. compare(a, b) == true && compare(b, c) == true -> compare(a, c) ==
    //   true
    // if it does not, std::sort will corrupt the heap.
    // NOTE to future self:
    // do not try to be smart; the same problem broke the trackfinder.
    auto compare = [](const std::unique_ptr<Hit>& hptr0,
                      const std::unique_ptr<Hit>& hptr1) {
      const auto& hit0 = *hptr0;
      const auto& hit1 = *hptr1;
      // 1. sort by value, highest first
      if (hit0.value() > hit1.value())
        return true;
      if (hit1.value() > hit0.value())
        return false;
      // 2. sort by timestamp, lowest first
      if (hit0.timestamp() < hit1.timestamp())
        return true;
      if (hit1.timestamp() < hit0.timestamp())
        return false;
      // equivalent hits w/ respect to value and time
      return false;
    };
    std::sort(clusterBegin, clusterEnd, compare);

    // add cluster to event
    auto& cluster =
        sensorEvent.addCluster(makeCluster(clusterBegin, clusterEnd));
    for (auto hit = clusterBegin; hit != clusterEnd; ++hit) {
      cluster.addHit(*hit->get());
    }

    // only consider the remaining hits for the next cluster
    clusterBegin = clusterEnd;
  }
}

std::string BinaryClusterizer::name() const
{
  return "BinaryClusterizer(" + m_sensor.name() + ")";
}

void BinaryClusterizer::execute(SensorEvent& sensorEvent) const
{
  auto makeCluster = [](auto h0, auto h1) {
    Scalar col = 0;
    Scalar row = 0;
    int ts = std::numeric_limits<int>::max();
    int value = 0;
    int size = 0;
    DigitalRange rangeCol = DigitalRange::Empty();
    DigitalRange rangeRow = DigitalRange::Empty();

    for (; h0 != h1; ++h0) {
      const Hit& hit = *(h0->get());
      col += hit.col();
      row += hit.row();
      ts = std::min(ts, hit.timestamp());
      value += hit.value();
      size += 1;
      rangeCol.enclose(DigitalRange(hit.col(), hit.col() + 1));
      rangeRow.enclose(DigitalRange(hit.row(), hit.row() + 1));
    }
    col /= size;
    row /= size;

    auto colVar = kVar / rangeCol.length();
    auto rowVar = kVar / rangeRow.length();
    auto tsVar = kVar;
    return Cluster(col, row, ts, value, colVar, rowVar, tsVar);
  };
  clusterize(m_sensor.pixelMask(), sensorEvent, sensorEvent.m_hits.begin(),
             sensorEvent.m_hits.end(), makeCluster);
}

std::string ValueWeightedClusterizer::name() const
{
  return "ValueWeightedClusterizer(" + m_sensor.name() + ")";
}

void ValueWeightedClusterizer::execute(SensorEvent& sensorEvent) const
{
  auto makeCluster = [](auto h0, auto h1) {
    Scalar col = 0;
    Scalar row = 0;
    int ts = std::numeric_limits<int>::max();
    int value = 0;
    DigitalRange rangeCol = DigitalRange::Empty();
    DigitalRange rangeRow = DigitalRange::Empty();

    for (; h0 != h1; ++h0) {
      const Hit& hit = *(h0->get());
      // TODO how to handle zero value?
      col += hit.value() * hit.col();
      row += hit.value() * hit.row();
      ts = std::min(ts, hit.timestamp());
      value += hit.value();
      rangeCol.enclose(DigitalRange(hit.col(), hit.col() + 1));
      rangeRow.enclose(DigitalRange(hit.row(), hit.row() + 1));
    }
    col /= value;
    row /= value;

    auto colVar = kVar / rangeCol.length();
    auto rowVar = kVar / rangeRow.length();
    auto tsVar = kVar;
    return Cluster(col, row, ts, value, colVar, rowVar, tsVar);
  };
  clusterize(m_sensor.pixelMask(), sensorEvent, sensorEvent.m_hits.begin(),
             sensorEvent.m_hits.end(), makeCluster);
}

std::string FastestHitClusterizer::name() const
{
  return "FastestHitClusterizer(" + m_sensor.name() + ")";
}

void FastestHitClusterizer::execute(SensorEvent& sensorEvent) const
{
  auto makeCluster = [](auto h0, auto h1) {
    Scalar col = 0;
    Scalar row = 0;
    int ts = std::numeric_limits<int>::max();
    int value = 0;

    for (; h0 != h1; ++h0) {
      const Hit& hit = *(h0->get());
      if (hit.timestamp() < ts) {
        col = hit.col();
        row = hit.row();
        ts = hit.timestamp();
        value = hit.value();
      }
    }

    return Cluster(col, row, ts, value, kVar, kVar, kVar);
  };
  clusterize(m_sensor.pixelMask(), sensorEvent, sensorEvent.m_hits.begin(),
             sensorEvent.m_hits.end(), makeCluster);
}

} // namespace proteus
