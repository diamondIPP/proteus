// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "matcher.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <vector>

#include "mechanics/device.h"
#include "storage/event.h"

namespace proteus {

Matcher::Matcher(const Device& device, Index sensorId, double distanceSigmaMax)
    : m_sensorId(sensorId)
    , m_distSquaredMax(
          (distanceSigmaMax < 0) ? -1 : (distanceSigmaMax * distanceSigmaMax))
    , m_name("Matcher(" + device.getSensor(sensorId).name() + ')')
{
}

std::string Matcher::name() const { return m_name; }

namespace {
struct PossibleMatch {
  Index cluster;
  Index track;
  double d2;
};
} // namespace

void Matcher::execute(Event& event) const
{
  SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  std::vector<PossibleMatch> possibleMatches;
  std::set<Index> matchedClusters;
  std::set<Index> matchedStates;

  // preselect possible track state / cluster pairs
  for (const auto& state : sensorEvent.localStates()) {
    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      const Cluster& cluster = sensorEvent.getCluster(icluster);

      // compute mahalanobis distance between state/cluster
      Vector2 delta(cluster.u() - state.loc0(), cluster.v() - state.loc1());
      SymMatrix2 cov = cluster.uvCov() + state.loc01Cov();
      double d2 = mahalanobisSquared(cov, delta);

      if ((m_distSquaredMax < 0) || (d2 < m_distSquaredMax))
        possibleMatches.emplace_back(
            PossibleMatch{icluster, state.track(), d2});
    }
  }
  // sort by pair distance, closest distance first
  std::sort(possibleMatches.begin(), possibleMatches.end(),
            [](const PossibleMatch& a, const PossibleMatch& b) {
              return (a.d2 < b.d2);
            });
  // select unique matches, closest distance first
  for (const auto& match : possibleMatches) {
    if ((0 < matchedClusters.count(match.cluster)) ||
        (0 < matchedStates.count(match.track)))
      continue;
    matchedClusters.insert(match.cluster);
    matchedStates.insert(match.track);
    sensorEvent.addMatch(match.cluster, match.track);
  }
}

} // namespace proteus
