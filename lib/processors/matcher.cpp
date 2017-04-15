#include "matcher.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <vector>

#include "mechanics/device.h"
#include "storage/event.h"

Processors::Matcher::Matcher(const Mechanics::Device& device,
                             Index sensorId,
                             double distanceSigmaMax)
    : m_sensorId(sensorId)
    , m_distSquaredMax(
          (distanceSigmaMax < 0) ? -1 : (distanceSigmaMax * distanceSigmaMax))
    , m_name("Matcher(" + device.getSensor(sensorId)->name() + ')')
{
}

std::string Processors::Matcher::name() const { return m_name; }

namespace {
struct PossibleMatch {
  Index state;
  Index cluster;
  double d2;
};
} // namespace

void Processors::Matcher::process(Storage::Event& event) const
{
  Storage::SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  std::vector<PossibleMatch> possibleMatches;
  std::set<Index> matchedStates;
  std::set<Index> matchedClusters;

  // preselect possible track state / cluster pairs
  for (Index istate = 0; istate < sensorEvent.numStates(); ++istate) {
    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      const auto& state = sensorEvent.getState(istate);
      const auto& cluster = *sensorEvent.getCluster(icluster);

      // compute mahalanobis distance between state/cluster
      XYVector delta = cluster.posLocal() - state.offset();
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      double d2 = mahalanobisSquared(cov, delta);

      if ((m_distSquaredMax < 0) || (d2 < m_distSquaredMax))
        possibleMatches.emplace_back(PossibleMatch{istate, icluster, d2});
    }
  }
  // sort by pair distance, closest distance first
  std::sort(possibleMatches.begin(), possibleMatches.end(),
            [](const auto& a, const auto& b) { return (a.d2 < b.d2); });
  // select unique matches, closest distance first
  for (const auto& match : possibleMatches) {
    if ((0 < matchedStates.count(match.state)) ||
        (0 < matchedClusters.count(match.cluster)))
      continue;
    matchedStates.insert(match.state);
    matchedClusters.insert(match.cluster);
    sensorEvent.getCluster(match.cluster)->setMatchedState(match.state);
    sensorEvent.getState(match.state).setMatchedCluster(match.cluster);
  }
}
