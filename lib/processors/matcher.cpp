#include "matcher.h"

#include <algorithm>
#include <cassert>
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

struct Pair {
  Storage::TrackState* state;
  Storage::Cluster* cluster;

  double d2() const
  {
    XYVector delta = cluster->posLocal() - state->offset();
    SymMatrix2 cov = cluster->covLocal() + state->covOffset();
    return mahalanobisSquared(cov, delta);
  }
};

struct PairDistanceCompare {
  bool operator()(const Pair& a, const Pair& b) const
  {
    return a.d2() < b.d2();
  }
};

// match a pair if both elements are still unmatched to anything
struct PairMatchUnmatched {
  void operator()(Pair& a) const
  {
    if (a.state->matchedCluster() || a.cluster->matchedState())
      return;
    a.state->setMatchedCluster(a.cluster);
    a.cluster->setMatchedState(a.state);
  }
};

void Processors::Matcher::process(Storage::Event& event) const
{
  std::vector<Pair> pairs;
  Storage::Plane& plane = *event.getPlane(m_sensorId);

  pairs.clear();
  pairs.reserve(plane.numStates() * plane.numClusters());

  // preselect possible track state / cluster pairs
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      Pair pair = {&plane.getState(istate), plane.getCluster(icluster)};

      if ((m_distSquaredMax < 0) || (pair.d2() < m_distSquaredMax)) {
        pairs.push_back(pair);
      }
    }
  }

  // sort by pair distance, closest distance first
  std::sort(pairs.begin(), pairs.end(), PairDistanceCompare());
  // select unique matches, closest distance first
  std::for_each(pairs.begin(), pairs.end(), PairMatchUnmatched());
}
