#include "trackfinder.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/linefitter.h"
#include "tracking/propagation.h"
#include "utils/logger.h"

using Mechanics::Plane;
using Storage::Cluster;
using Storage::Event;
using Storage::SensorEvent;
using Storage::Track;
using Storage::TrackState;
using Tracking::propagateTo;

PT_SETUP_LOCAL_LOGGER(TrackFinder)

Tracking::TrackFinder::TrackFinder(const Mechanics::Device& device,
                                   std::vector<Index> trackingIds,
                                   size_t sizeMin,
                                   double searchSigmaMax,
                                   double redChi2Max)
    // 2-d Mahalanobis distance peaks at 2 and not at 1
    : m_d2Max((0 < searchSigmaMax) ? (2 * searchSigmaMax * searchSigmaMax) : -1)
    , m_reducedChi2Max(redChi2Max)
{
  if (trackingIds.size() < 2) {
    throw std::runtime_error("Need at least two sensors two find tracks");
  }
  if (trackingIds.size() < sizeMin) {
    throw std::runtime_error(
        "Number of tracking sensors < minimum number of clusters");
  }
  // ensure the requested tracking sensors are unique
  std::sort(trackingIds.begin(), trackingIds.end());
  if (std::unique(trackingIds.begin(), trackingIds.end()) !=
      trackingIds.end()) {
    throw std::runtime_error("Found duplicate tracking sensor ids");
  }
  // ensure the requested tracking sensors are valid
  std::vector<Index> allIds = device.sensorIds();
  std::sort(allIds.begin(), allIds.end());
  if (!std::includes(allIds.begin(), allIds.end(), trackingIds.begin(),
                     trackingIds.end())) {
    throw std::runtime_error("Found invalid tracking sensor ids");
  }

  // Build the search steps along the beam direction.
  //
  // Ingore sensors before and after, but keep unused intermediates onces.
  // These are e.g. devices-under-test that do not provide measurements but
  // give rise to additional uncertainty from material interactions.

  // Determine the range of sensors to be searched/propagated to.
  Mechanics::sortAlongBeam(device.geometry(), trackingIds);
  Mechanics::sortAlongBeam(device.geometry(), allIds);
  auto first = std::find(allIds.begin(), allIds.end(), trackingIds.front());
  auto last = std::next(std::find(first, allIds.end(), trackingIds.back()));

  size_t remainingTrackingSensors = trackingIds.size();
  size_t remainingSeedSensors = 1 + (trackingIds.size() - sizeMin);
  for (; first != last; ++first) {
    Step step;

    // geometry and propagation uncertainty is always needed
    step.plane = device.geometry().getPlane(*first);
    // TODO add multiple scattering
    step.processNoise = SymMatrix6::Zero();
    step.sensorId = *first;

    // check if the sensor is a tracking sensor or just dead material
    if ((0 < remainingTrackingSensors) and
        std::find(trackingIds.begin(), trackingIds.end(), step.sensorId) !=
            trackingIds.end()) {
      step.useForTracking = true;
      remainingTrackingSensors -= 1;

      // the first n tracking sensors are also used for seeding
      if (0 < remainingSeedSensors) {
        step.useForSeeding = true;
        // use beam information for seed direction
        step.seedSlope = device.geometry().getBeamSlope(step.sensorId);
        step.seedSlopeCovariance =
            device.geometry().getBeamSlopeCovariance(step.sensorId);
        remainingSeedSensors -= 1;
      }
    }
    step.candidateSizeMin = (remainingTrackingSensors < sizeMin)
                                ? (sizeMin - remainingTrackingSensors)
                                : 0;

    m_steps.push_back(std::move(step));
  }

  // (debug) output
  for (const auto& step : m_steps) {
    const auto& sensor = device.getSensor(step.sensorId);
    if (step.useForTracking) {
      if (step.useForSeeding) {
        VERBOSE(sensor.name(), " id=", sensor.id(), " is a seeding plane");
      } else {
        VERBOSE(sensor.name(), " id=", sensor.id(), " is a tracking plane");
      }
    } else {
      VERBOSE(sensor.name(), " id=", sensor.id(), " is dead material");
    }
    DEBUG("  minimum candidate size: ", step.candidateSizeMin);
    DEBUG("  process noise:\n", format(step.processNoise));
  }
}

std::string Tracking::TrackFinder::name() const { return "TrackFinder"; }

// Propagate all states from the previous plane to the current plane.
// This incorporates uncertainties from material interactions.
static void propagateToCurrent(const SymMatrix6& processNoise,
                               const Plane& previousPlane,
                               const Plane& currentPlane,
                               std::vector<Track>& candidates)
{
  for (auto& track : candidates) {
    // include material interactions at the prev plane
    TrackState state(track.globalState().params(),
                     track.globalState().cov() + processNoise);
    state = propagateTo(state, previousPlane, currentPlane);
    track.setGlobalState(state);
  }
}

// Propagate all states from the local plane into the global plane.
static void propagateToGlobal(const Plane& local,
                              std::vector<Track>& candidates)
{
  for (auto& track : candidates) {
    // no additional uncertainty since we want the equivalent state
    // default plane constructor yields the global plane
    TrackState state = propagateTo(track.globalState(), local, Plane{});
    track.setGlobalState(state);
  }
}

// Search for matching clusters for all candidates on the given sensor.
//
// Ambiguities are not resolved but result in additional track candidates.
// Track states are updated using the Kalman filter method based on the
// additional information from the added cluster.
static void searchSensor(Scalar d2Max,
                         Index sensorId,
                         const SensorEvent& sensorEvent,
                         std::vector<Track>& candidates,
                         std::vector<bool>& clusterMask)
{
  // Projection from track state into local measurement plane
  // TODO can we just use a projection matrix?
  // TODO extend to time
  Matrix26 H = Matrix26::Zero();
  H(kU, kLoc0) = 1;
  H(kV, kLoc1) = 1;

  // loop only over the initial candidates and not the added ones
  //
  // WARNING
  // we are modifying the list of candidates while iterating over it.
  // always access candidates through the index and not via a reference or
  // an iterator. If the underlying memory gets reallocated we will access
  // random memory and break the heap (and you will spent about a day
  // trying to figure out why a call to std::map segfaults).
  size_t numTracks = candidates.size();
  for (size_t itrack = 0; itrack < numTracks; ++itrack) {
    TrackState state = candidates[itrack].globalState();
    Scalar chi2 = candidates[itrack].chi2();
    int numMatchedClusters = 0;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      auto& cluster = sensorEvent.getCluster(icluster);
      if (cluster.isInTrack()) {
        clusterMask[icluster] = true;
        continue;
      }

      // predicted residuals and covariance
      Vector2 r = cluster.uv() - H * state.params();
      Matrix2 R = cluster.uvCov() + transformCovariance(H, state.cov());

      // check if the cluster is compatible
      if ((0 <= d2Max) and (d2Max < mahalanobisSquared(R, r))) {
        continue;
      }

      // update track state using this cluster
      // Kalman gain matrix
      Matrix<Scalar, 6, 2> K = state.cov() * H.transpose() * R.inverse();
      // filtered local state and covariance
      // use numerically superior (but slower formula) for covariance update
      Vector6 xf = state.params() + K * r;
      SymMatrix6 Cf =
          transformCovariance(SymMatrix6::Identity() - K * H, state.cov()) +
          transformCovariance(K, cluster.uvCov());
      // filtered residuals and covariance
      r = cluster.uv() - H * xf;
      R = cluster.uvCov() - transformCovariance(H, Cf);
      // resulting change in chi²
      Scalar chi2Update = mahalanobisSquared(R, r);

      if (numMatchedClusters == 0) {
        // first matched cluster; update existing track
        auto& track = candidates[itrack];
        track.setGlobalState(xf, Cf);
        track.setGoodnessOfFit(chi2 + chi2Update, 2 * track.size() - 6);
        track.addCluster(sensorId, icluster);
      } else {
        // additional matched cluster; duplicate track w/ different content
        Track track = candidates[itrack];
        track.setGlobalState(xf, Cf);
        track.setGoodnessOfFit(chi2 + chi2Update, 2 * track.size() - 6);
        // this replaces a previous cluster for the same sensor
        track.addCluster(sensorId, icluster);
        candidates.push_back(std::move(track));
      }
      numMatchedClusters += 1;

      // mark cluster as in-use for the seeding.
      clusterMask[icluster] = true;
    }
  }

  auto numClusters = std::count(clusterMask.begin(), clusterMask.end(), true);
  auto numBifurcations = (candidates.size() - numTracks);
  if (0 < numBifurcations) {
    DEBUG("sensor ", sensorId, " added ", numClusters, " clusters to ",
          numTracks, "+", numBifurcations, " candidates+bifurcations");
  } else {
    DEBUG("sensor ", sensorId, " added ", numClusters, " clusters to ",
          numTracks, " candidates");
  }
}

static void makeSeedsFromUnusedClusters(Index sensorId,
                                        const SensorEvent& sensorEvent,
                                        const std::vector<bool>& clusterMask,
                                        const Vector2& seedSlope,
                                        const SymMatrix2& seedSlopeCovariance,
                                        std::vector<Track>& candidates)
{
  size_t numSeeds = 0;

  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    if (clusterMask[icluster]) {
      continue;
    }
    const auto& cluster = sensorEvent.getCluster(icluster);
    // abuse global state to store the state on the current plane
    TrackState seedState(cluster.position(), cluster.positionCov(), seedSlope,
                         seedSlopeCovariance);
    // no fit yet -> no chi2, undefined degrees-of-freedom
    candidates.emplace_back(seedState, 0, -1);
    candidates.back().addCluster(sensorId, icluster);
    numSeeds += 1;
  }

  DEBUG("sensor ", sensorId, " added ", numSeeds, " seeds");
}

// remove track candidates that are too short
static void removeShortCandidates(size_t sizeMin,
                                  std::vector<Track>& candidates)
{
  auto isTooShort = [=](const Track& t) { return t.size() < sizeMin; };
  auto rm = std::remove_if(candidates.begin(), candidates.end(), isTooShort);
  auto n = std::distance(rm, candidates.end());
  candidates.erase(rm, candidates.end());

  if (0 < n) {
    DEBUG("removed ", n, " short candidates");
  }
}

// remove candidates that do not pass quality cuts
static void removeBadCandidates(Scalar reducedChi2Max,
                                std::vector<Track>& candidates)
{
  // drop bad candidates
  auto isBad = [=](const Track& t) {
    // numerical crosschecks
    if (t.degreesOfFreedom() < 0) {
      return true;
    }
    if (not std::isfinite(t.chi2())) {
      return true;
    }
    if (not std::isfinite(t.reducedChi2())) {
      return true;
    }
    // negative value disables the cut
    if ((0 < reducedChi2Max) and (reducedChi2Max <= t.reducedChi2())) {
      return true;
    }
    return false;
  };
  auto rm = std::remove_if(candidates.begin(), candidates.end(), isBad);
  auto n = std::distance(rm, candidates.end());
  candidates.erase(rm, candidates.end());

  if (0 < n) {
    DEBUG("removed ", n, " bad candidates");
  }
}

// sort longest tracks w/ smallest chi2 first
static void sortCandidates(std::vector<Track>& candidates)
{
  std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) {
              return (a.size() > b.size()) or (a.chi2() < b.chi2());
            });
}

// add all tracks w/ exclusive cluster-to-track association to the event
static void addTracksToEvent(const std::vector<Track>& candidates, Event& event)
{
  size_t numAddedTracks = 0;

  for (const auto& track : candidates) {
    // all clusters of the track must be unused
    bool hasUsedClusters = std::any_of(
        track.clusters().begin(), track.clusters().end(), [&](const auto& c) {
          return event.getSensorEvent(c.sensor)
              .getCluster(c.cluster)
              .isInTrack();
        });
    if (hasUsedClusters) {
      continue;
    }
    // add new, good track to the event; also fixes cluster-track association
    event.addTrack(track);
    numAddedTracks += 1;
  }

  DEBUG(numAddedTracks, " tracks added to event");
}

void Tracking::TrackFinder::execute(Event& event) const
{
  std::vector<Track> candidates;
  std::vector<bool> clusterMask;

  for (size_t istep = 0; istep < m_steps.size(); ++istep) {
    const auto& curr = m_steps[istep];
    auto& sensorEvent = event.getSensorEvent(curr.sensorId);
    // initialize usage mask, all clusters are assumed unused at the beginning
    clusterMask.assign(sensorEvent.numClusters(), false);

    // propagate states onto the current plane w/ material effects
    if (0 < istep) {
      const auto& prev = m_steps[istep - 1];
      propagateToCurrent(prev.processNoise, prev.plane, curr.plane, candidates);
    }

    // search for compatible clusters only on tracking planes.
    // by design, there are no candidates on the first one plane.
    if ((0 < istep) and (curr.useForTracking)) {
      // updates/extends candidates and sets clustersUsed flags
      searchSensor(m_d2Max, curr.sensorId, sensorEvent, candidates,
                   clusterMask);
      // ignore candidates that can never fullfill the final size cut
      removeShortCandidates(curr.candidateSizeMin, candidates);
    }

    // generate track candidates from unused clusters on seeding planes
    // this has to happen last so clusters are picked up first by existing
    // candidates generated on earlier seeding planes.
    if (curr.useForSeeding) {
      makeSeedsFromUnusedClusters(curr.sensorId, sensorEvent, clusterMask,
                                  curr.seedSlope, curr.seedSlopeCovariance,
                                  candidates);
    }
  }

  // final track selection and transformations
  removeBadCandidates(m_reducedChi2Max, candidates);
  sortCandidates(candidates);
  propagateToGlobal(m_steps.back().plane, candidates);
  addTracksToEvent(candidates, event);
}
