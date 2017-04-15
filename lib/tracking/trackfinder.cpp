#include "trackfinder.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/tracking.h"
#include "utils/logger.h"

using Storage::Cluster;
using Storage::Event;
using Storage::SensorEvent;
using Storage::Track;
using Storage::TrackState;

PT_SETUP_GLOBAL_LOGGER

Tracking::TrackFinder::TrackFinder(const Mechanics::Device& device,
                                   std::vector<Index> sensors,
                                   Index numClustersMin,
                                   double searchSigmaMax,
                                   double redChi2Max)
    : m_sensorIds(std::move(sensors))
    , m_numClustersMin(numClustersMin)
    // 2-d Mahalanobis distance peaks at 2 and not at 1
    , m_d2Max((searchSigmaMax < 0) ? -1 : (2 * searchSigmaMax * searchSigmaMax))
    , m_redChi2Max(redChi2Max)
    , m_beamDirection(device.geometry().beamDirection())
{
  if (m_sensorIds.size() < 2)
    throw std::runtime_error("Need at least two sensors two find tracks");
  if (m_sensorIds.size() < m_numClustersMin)
    throw std::runtime_error(
        "Number of tracking sensors < minimum number of clusters");
  // TODO 2016-11 msmk: check that sensor ids are unique
}

std::string Tracking::TrackFinder::name() const { return "TrackFinder"; }

void Tracking::TrackFinder::process(Storage::Event& event) const
{
  std::vector<TrackPtr> candidates;

  // first iteration over all seed sensors
  size_t numSeedSeensors = 1 + (m_sensorIds.size() - m_numClustersMin);
  for (size_t i = 0; i < numSeedSeensors; ++i) {
    SensorEvent& seedEvent = event.getSensorEvent(m_sensorIds[i]);

    // generate track candidates from all unused clusters on the seed sensor
    candidates.clear();
    for (Index icluster = 0; icluster < seedEvent.numClusters(); ++icluster) {
      Cluster* cluster = seedEvent.getCluster(icluster);
      if (cluster->isInTrack())
        continue;
      candidates.push_back(TrackPtr(new Track()));
      candidates.back()->addCluster(cluster);
    }

    // second iteration over remaining sensors to find compatible points
    for (size_t j = i + 1; j < m_sensorIds.size(); ++j) {
      searchSensor(event.getSensorEvent(m_sensorIds[j]), candidates);

      // remove seeds that are already too short
      Index remainingSensors = m_sensorIds.size() - (j + 1);
      auto isLongEnough = [&](const TrackPtr& cand) {
        return (m_numClustersMin <= (cand->numClusters() + remainingSensors));
      };
      auto beginBad =
          std::partition(candidates.begin(), candidates.end(), isLongEnough);
      candidates.erase(beginBad, candidates.end());
    }

    // select final tracks after all possible candidates have been found to
    // allow a more global selection of track candidates.
    selectTracks(candidates, event);

    // TODO 2017-05-10 msmk:
    // ideally we would find all candidates starting from all seed planes first
    // and only then select the final tracks. this should avoid cases where
    // a bad 6-hit track, e.g. with one noise hit, shadows a good 5-hit track
    // with the same cluster content.
    // this would require a combined metric that sorts both by number of hits
    // and by track quality. defining such a combined metric is always a bit
    // arbitrary and introduces additional corner cases that i would prefer
    // to avoid.
    // just sorting by chi2 along results yields 50% 5-hit tracks in the
    // test samples; compared to 90% 6-hit tracks with the default algorithm.
    // a single misaligned sensor will always result in a better track with
    // less hits.
  }
}

/** Search for matching clusters for all candidates on the given sensor.
 *
 * Ambiguities are not resolved but result in additional track candidates.
 */
void Tracking::TrackFinder::searchSensor(
    Storage::SensorEvent& sensorEvent, std::vector<TrackPtr>& candidates) const
{
  // loop only over the initial candidates and not the added ones
  Index numTracks = static_cast<Index>(candidates.size());
  for (Index itrack = 0; itrack < numTracks; ++itrack) {
    Storage::Track& track = *candidates[itrack];
    Storage::Cluster* last = track.getCluster(track.numClusters() - 1);
    Storage::Cluster* matched = nullptr;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      Storage::Cluster* curr = sensorEvent.getCluster(icluster);

      // clusters already in use must be ignored
      if (curr->isInTrack())
        continue;

      XYZVector delta = curr->posGlobal() - last->posGlobal();
      delta -= delta.z() * m_beamDirection;
      SymMatrix2 cov = last->covGlobal().Sub<SymMatrix2>(0, 0) +
                       curr->covGlobal().Sub<SymMatrix2>(0, 0);
      double d2 = mahalanobisSquared(cov, Vector2(delta.x(), delta.y()));

      if ((0 < m_d2Max) && (m_d2Max < d2))
        continue;

      if (matched == nullptr) {
        // first matching cluster
        matched = curr;
      } else {
        // matching ambiguity -> bifurcate track
        candidates.push_back(TrackPtr(new Track(track)));
        candidates.back()->addCluster(curr);
      }
    }
    // first matched cluster can be only be added after all other clusters
    // have been considered. otherwise it would be already added to the
    // candidate when it bifurcates and the new candidate would have two
    // clusters on this sensor.
    if (matched)
      track.addCluster(matched);
  }
}

// compare tracks by number of clusters and chi2. high n, low chi2 comes first
struct CompareNumClusterChi2 {
  bool operator()(const std::unique_ptr<Storage::Track>& a,
                  const std::unique_ptr<Storage::Track>& b)
  {
    if (a->numClusters() == b->numClusters())
      return (a->reducedChi2() < b->reducedChi2());
    return (b->numClusters() < a->numClusters());
  }
};

/** Add tracks selected by chi2 and unique cluster association to the event. */
void Tracking::TrackFinder::selectTracks(std::vector<TrackPtr>& candidates,
                                         Storage::Event& event) const
{
  // ensure chi2 value is up-to-date
  std::for_each(candidates.begin(), candidates.end(),
                [&](TrackPtr& t) { fitTrackGlobal(*t); });
  // sort good candidates first, i.e. longest track and smallest chi2
  std::sort(candidates.begin(), candidates.end(), CompareNumClusterChi2());

  // fix cluster assignment starting w/ best tracks first
  for (auto& track : candidates) {

    // apply track cuts
    if ((0 < m_redChi2Max) && (m_redChi2Max < track->reducedChi2()))
      continue;

    // check that all constituent clusters are still unused
    Index unique = 0;
    for (Index icluster = 0; icluster < track->numClusters(); ++icluster)
      unique +=
          ((track->getCluster(icluster)->isInTrack()) ? 1 : 0);
    // some clusters are already used by other tracks
    if (unique != track->numClusters())
      continue;

    // this is a good track
    track->freezeClusterAssociation();
    event.addTrack(TrackPtr(track.release()));
  }
}
