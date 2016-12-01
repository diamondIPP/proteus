#include "matcher.h"

#include <algorithm>
#include <cassert>
#include <vector>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"

Processors::Matcher::Matcher(const Mechanics::Device& device,
                             Index sensorId,
                             double distanceSigmaMax)
    : m_sensorId(sensorId)
    , m_distSquaredMax(
          (distanceSigmaMax < 0) ? -1 : (distanceSigmaMax * distanceSigmaMax))
    , m_name("Matcher_" + device.getSensor(sensorId)->name())
{
}

std::string Processors::Matcher::name() const { return m_name; }

struct Pair {
  Storage::Track* track;
  Storage::Cluster* cluster;

  bool distanceSquared(Index sensorId) const
  {
    const Storage::TrackState& state = track->getLocalState(sensorId);
    XYVector delta = cluster->posLocal() - state.offset();
    SymMatrix2 cov = cluster->covLocal() + state.covOffset();
    return mahalanobisSquared(cov, delta);
  }
};

struct PairDistanceCmp {
  Index sensorId;

  bool operator()(const Pair& a, const Pair& b) const
  {
    return a.distanceSquared(sensorId) < b.distanceSquared(sensorId);
  }
};

void Processors::Matcher::process(Storage::Event& event) const
{
  std::vector<Pair> pairs;
  Storage::Plane& plane = *event.getPlane(m_sensorId);

  pairs.reserve(event.numTracks() * plane.numClusters());

  // list all (reasonable) track/cluster combinations
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track* track = event.getTrack(itrack);
    if (!track->hasLocalState(m_sensorId))
      continue;
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      Pair pair = {track, plane.getCluster(icluster)};
      // preselect by distance if a distance cut is given
      if ((m_distSquaredMax < 0) ||
          (pair.distanceSquared(m_sensorId) < m_distSquaredMax)) {
        pairs.push_back(pair);
      }
      // pairs.push_back(pair);
    }
  }

  // sort by pair distance, closest distance first
  std::sort(pairs.begin(), pairs.end(), PairDistanceCmp{m_sensorId});

  // select unique matches, closest distance first
  for (auto pair = pairs.begin(); pair != pairs.end(); ++pair) {
    Storage::Track* track = pair->track;
    Storage::Cluster* cluster = pair->cluster;
    // ignore any pairs where either part is already matched
    if (track->hasMatchedCluster(m_sensorId) || cluster->hasMatchedTrack())
      continue;
    // fix matching
    track->setMatchedCluster(m_sensorId, cluster);
    cluster->setMatchedTrack(track);
  }
}

Processors::TrackMatcher::TrackMatcher(const Mechanics::Device* device)
    : _device(device)
{
}

void Processors::TrackMatcher::matchEvent(Storage::Event* refEvent,
                                          Storage::Event* dutEvent)
{
  assert(refEvent && dutEvent && "TrackMatching: null event and/or device");
  assert(dutEvent->getNumPlanes() == _device->getNumSensors() &&
         "TrackMatching: plane / sensor mis-match");

  // Look for a tracks to clusters in each plane
  for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++) {
    Storage::Plane* plane = dutEvent->getPlane(nplane);
    const Mechanics::Sensor* sensor = _device->getSensor(nplane);

    // If there are more cluster than tracks, match each track to one cluster
    if (plane->numClusters() >= refEvent->numTracks())
      matchTracksToClusters(refEvent, plane, sensor);
    else
      matchClustersToTracks(refEvent, plane, sensor);
  }
}

void Processors::TrackMatcher::matchTracksToClusters(
    Storage::Event* trackEvent,
    Storage::Plane* clustersPlane,
    const Mechanics::Sensor* clustersSensor)
{
  // Look for matches for all tracks
  for (unsigned int ntrack = 0; ntrack < trackEvent->getNumTracks(); ntrack++) {
    Storage::Track* track = trackEvent->getTrack(ntrack);

    // Find the nearest cluster
    double nearestDist = 0;
    Storage::Cluster* match = 0;

    for (unsigned int ncluster = 0; ncluster < clustersPlane->numClusters();
         ncluster++) {
      Storage::Cluster* cluster = clustersPlane->getCluster(ncluster);

      double x = 0, y = 0, errx = 0, erry = 0;
      trackClusterDistance(track, cluster, clustersSensor, x, y, errx, erry);

      // const double dist = sqrt(pow(x / errx, 2) + pow(y / erry, 2));

      // bilbao@cern.ch: circular distance to calculate the track-cluster
      // distance
      // fdibello@cern.ch: definition of the distance that takes in account the
      // rectangulatr shape of pixel of the DUT
      const double dist = sqrt(pow(x / clustersSensor->getPitchX(), 2) +
                               pow(y / clustersSensor->getPitchY(), 2));

      if (!match || dist < nearestDist) {
        nearestDist = dist;
        match = cluster;
      }
    }

    // If is a match, store this in the event
    if (match) {
      track->setMatchedCluster(clustersPlane->sensorId(), match);
      match->setMatchedTrack(track);
      match->setMatchDistance(nearestDist);
    }
  }
}

void Processors::TrackMatcher::matchClustersToTracks(
    Storage::Event* trackEvent,
    Storage::Plane* clustersPlane,
    const Mechanics::Sensor* clustersSensor)
{

  for (unsigned int ncluster = 0; ncluster < clustersPlane->numClusters();
       ncluster++) {
    Storage::Cluster* cluster = clustersPlane->getCluster(ncluster);

    // Find the nearest track
    double nearestDist = 0;
    Storage::Track* match = 0;

    // Look for matches for all tracks
    for (unsigned int ntrack = 0; ntrack < trackEvent->getNumTracks();
         ntrack++) {
      Storage::Track* track = trackEvent->getTrack(ntrack);

      // fdibello@cern.ch
      // Mechanics::Sensor* sensor = device->getSensor(clustersPlane);

      double x = 0, y = 0, errx = 0, erry = 0;
      trackClusterDistance(track, cluster, clustersSensor, x, y, errx, erry);

      // const double dist = sqrt(pow(x / errx, 2) + pow(y / erry, 2));

      // bilbao@cern.ch: cone distance to calculate the track-cluster distance
      const double dist = sqrt(pow(x / clustersSensor->getPitchX(), 2) +
                               pow(y / clustersSensor->getPitchY(), 2));

      if (!match || dist < nearestDist) {
        nearestDist = dist;
        match = track;
      }
    }

    // If is a match, store this in the event
    if (match) {
      match->setMatchedCluster(clustersPlane->sensorId(), cluster);
      cluster->setMatchedTrack(match);
      cluster->setMatchDistance(nearestDist);
    }
  }
}
