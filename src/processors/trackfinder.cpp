#include "trackfinder.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/event.h"
#include "utils/logger.h"

using Storage::Cluster;
using Storage::Event;
using Storage::Plane;
using Storage::Track;
using Storage::TrackState;

PT_SETUP_GLOBAL_LOGGER

Processors::TrackFinder::TrackFinder(const Mechanics::Device& device,
                                     const std::vector<Index> sensors,
                                     Index numClustersMin,
                                     double distanceSigmaMax)
    : m_sensors(sensors)
    , m_numSeedSensors(1 + sensors.size() - numClustersMin)
    , m_numClustersMin(numClustersMin)
    , m_distSigmaMax(distanceSigmaMax)
    , m_beamDirection(device.beamDirection())
{
  if (sensors.size() < 2)
    throw std::runtime_error("Need at least two sensors two find tracks");
  if (sensors.size() < numClustersMin)
    throw std::runtime_error(
        "Number of tracking sensors < minimum number of clusters");
  // TODO 2016-11 msmk: check that sensor ids are unique
}

std::string Processors::TrackFinder::name() const { return "TrackFinder"; }

void Processors::TrackFinder::process(Storage::Event& event) const
{
  std::vector<TrackPtr> candidates;

  // start a track search from each seed sensor.

  for (auto seed = m_sensors.begin();
       seed != m_sensors.begin() + m_numSeedSensors; ++seed) {
    Plane& seedSensorEvent = *event.getPlane(*seed);

    // generate track candidates from unused clusters on the seed sensor
    candidates.clear();
    for (Index icluster = 0; icluster < seedSensorEvent.numClusters();
         ++icluster) {
      Cluster* cluster = seedSensorEvent.getCluster(icluster);
      if (cluster->isInTrack())
        continue;
      candidates.push_back(TrackPtr(new Track()));
      candidates.back()->addCluster(cluster);
    }
    // search for additional hits on all other sensors
    for (auto id = m_sensors.begin(); id != m_sensors.end(); ++id) {
      // ignore seed sensor to prevent adding the same cluster twice
      if (*id == *seed)
        continue;
      searchSensor(*event.getPlane(*id), candidates);
    }
    selectTracks(candidates, event);
  }
}

/** Search for matching clusters for all candidates on the given sensor.
 *
 * Ambiguities are not resolved but result in additional track candidates.
 */
void Processors::TrackFinder::searchSensor(
    Storage::Plane& sensorEvent, std::vector<TrackPtr>& candidates) const
{
  // we must loop only over the initial candidates and not the added ones
  Index numTracks = static_cast<Index>(candidates.size());
  for (Index itrack = 0; itrack < numTracks; ++itrack) {
    Storage::Track& track = *candidates[itrack];
    Storage::Cluster* last = track.getCluster(track.numClusters() - 1);
    Storage::Cluster* matched = NULL;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      Storage::Cluster* curr = sensorEvent.getCluster(icluster);

      // clusters already in use must be ignored
      if (curr->isInTrack())
        continue;

      // TODO switch to full covariance matrix
      const double errX =
          sqrt(pow(curr->getPosErrX(), 2) + pow(last->getPosErrX(), 2));
      const double errY =
          sqrt(pow(curr->getPosErrY(), 2) + pow(last->getPosErrY(), 2));
      // projected global xy distance w/ beam slope correction
      XYZVector delta = curr->posGlobal() - last->posGlobal();
      delta -= delta.z() * m_beamDirection;
      // TODO use full mahalanobis distance
      double sigma = sqrt(pow(delta.x() / errX, 2) + pow(delta.y() / errY, 2));

      if (m_distSigmaMax < sigma)
        continue;

      if (matched == NULL) {
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

/** Add track selected by chi2 and unique cluster association to the event. */
void Processors::TrackFinder::selectTracks(std::vector<TrackPtr>& candidates,
                                           Storage::Event& event) const
{
  // ensure chi2 is up-to-date
  for (auto itrack = candidates.begin(); itrack != candidates.end(); ++itrack)
    Processors::fitTrack(**itrack);

  // sort by number of hits and chi2 value
  std::sort(candidates.begin(), candidates.end(), CompareNumClusterChi2());

  // fix cluster assignment starting w/ best tracks first
  for (auto itrack = candidates.begin(); itrack != candidates.end(); ++itrack) {
    Storage::Track& track = **itrack;

    // too short
    if (track.numClusters() < m_numClustersMin)
      continue;

    // check that all constituent clusters are still unused
    Index unique = 0;
    for (Index icluster = 0; icluster < track.numClusters(); ++icluster)
      unique += (track.getCluster(icluster)->isInTrack() ? 0 : 1);
    // some clusters are already used by other tracks
    if (unique != track.numClusters())
      continue;

    // this is a good track
    track.fixClusterAssociation();
    event.addTrack(TrackPtr(itrack->release()));
  }
}
