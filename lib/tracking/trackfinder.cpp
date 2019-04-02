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
                                   double searchSpatialSigmaMax,
                                   double searchTemporalSigmaMax,
                                   size_t sizeMin,
                                   double redChi2Max)
    // 2-d Mahalanobis distance peaks at 2 and not at 1
    : m_d2LocMax((0 < searchSpatialSigmaMax)
                     ? (2 * searchSpatialSigmaMax * searchSpatialSigmaMax)
                     : -1)
    , m_d2TimeMax((0 < searchTemporalSigmaMax)
                      ? (searchTemporalSigmaMax * searchTemporalSigmaMax)
                      : -1)
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
    // at the moment only multiple scattering is considered
    // TODO what else? beam energy spread? alignment uncertainty?
    step.processNoise = SymMatrix6::Zero();
    step.processNoise.block<2, 2>(kSlope, kSlope) =
        device.getSensor(*first).scatteringSlopeCovariance();
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

    // how large has a candidate be at this point to be viable?
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
static void searchSensor(Scalar d2LocMax,
                         Scalar d2TimeMax,
                         Index sensorId,
                         const SensorEvent& sensorEvent,
                         std::vector<Track>& candidates,
                         std::vector<bool>& usedClusters)
{
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
    // keep a copy; candidate state will be modified, but the original
    // state is needed to check for further compatible clusters.
    TrackState state = candidates[itrack].globalState();
    Scalar chi2 = candidates[itrack].chi2();
    int numMatchedClusters = 0;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      auto& cluster = sensorEvent.getCluster(icluster);

      // in principle, there could already be tracks in the event,
      // e.g. running multiple track finders with different settings, and we
      // should only consider free clusters. a bit academic, i know.
      if (cluster.isInTrack()) {
        usedClusters[icluster] = true;
        continue;
      }

      // predicted residuals and covariance
      Vector3 r = cluster.onPlane() - state.onPlane();
      SymMatrix3 R = cluster.onPlaneCov() + state.onPlaneCov();

      // check if the cluster is compatible in space
      Scalar d2Loc =
          mahalanobisSquared(R.block<2, 2>(kLoc0 - kOnPlane, kLoc0 - kOnPlane),
                             r.segment<2>(kLoc0));
      if ((0 <= d2LocMax) and (d2LocMax < d2Loc)) {
        continue;
      }
      // check if the cluster is compatible in time
      Scalar d2Time =
          mahalanobisSquared(R.block<1, 1>(kTime - kOnPlane, kTime - kOnPlane),
                             r.segment<1>(kTime - kOnPlane));
      if ((0 <= d2TimeMax) and (d2TimeMax < d2Time)) {
        continue;
      }

      // optimal Kalman gain matrix
      Matrix<Scalar, 6, 3> K =
          state.cov().block<6, 3>(0, kOnPlane) * R.inverse();
      // filtered local state and covariance
      TrackState filtered(state.params() + K * r,
                          state.cov() -
                              K * state.cov().block<3, 6>(kOnPlane, 0));
      // filtered residuals and covariance
      r = cluster.onPlane() - filtered.onPlane();
      R = cluster.onPlaneCov() - filtered.onPlaneCov();
      // chi^2 update
      Scalar chi2Update = mahalanobisSquared(R, r);

      if (numMatchedClusters == 0) {
        // first matched cluster; update existing track
        auto& track = candidates[itrack];
        track.addCluster(sensorId, icluster);
        track.setGlobalState(std::move(filtered));
        track.setGoodnessOfFit(chi2 + chi2Update, 3 * track.size() - 6);
      } else {
        // additional matched cluster; duplicate track w/ different content
        Track track = candidates[itrack];
        // this replaces any previously added cluster for the same sensor
        track.addCluster(sensorId, icluster);
        track.setGlobalState(std::move(filtered));
        track.setGoodnessOfFit(chi2 + chi2Update, 3 * track.size() - 6);
        candidates.push_back(std::move(track));
      }
      numMatchedClusters += 1;

      DEBUG("sensor ", sensorId, " added cluster ", icluster, " to candidate ",
            itrack, " w/ d2loc=", d2Loc, " d2time=", d2Time,
            " dchi2=", chi2Update);

      // mark cluster as in-use for the seeding.
      usedClusters[icluster] = true;
    }
  }

  auto numClusters = std::count(usedClusters.begin(), usedClusters.end(), true);
  auto numBifurcations = (candidates.size() - numTracks);
  if (0 < numBifurcations) {
    DEBUG("sensor ", sensorId, " added ", numClusters, " clusters to ",
          numTracks, "+", numBifurcations, " candidates+bifurcations");
  } else {
    DEBUG("sensor ", sensorId, " added ", numClusters, " clusters to ",
          numTracks, " candidates");
  }
}

// make seeds using the beam information for the initial direction
static void makeSeedsFromUnusedClusters(Index sensorId,
                                        const SensorEvent& sensorEvent,
                                        const Vector2& seedSlope,
                                        const SymMatrix2& seedSlopeCovariance,
                                        std::vector<Track>& candidates,
                                        std::vector<bool>& usedClusters)
{
  size_t numSeeds = 0;

  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    const auto& cluster = sensorEvent.getCluster(icluster);

    if (usedClusters[icluster]) {
      continue;
    }
    // in principle, there could already be tracks in the event,
    // e.g. running multiple track finders with different settings, and we
    // should only consider free clusters. a bit academic, i know.
    if (cluster.isInTrack()) {
      usedClusters[icluster] = true;
      continue;
    }

    // abuse global state to store the state on the current plane
    TrackState seedState(cluster.position(), cluster.positionCov(), seedSlope,
                         seedSlopeCovariance);
    // no fit yet -> no chi2, undefined degrees-of-freedom
    candidates.emplace_back(seedState, 0, -1);
    candidates.back().addCluster(sensorId, icluster);
    numSeeds += 1;
  }

  if (0 < numSeeds) {
    DEBUG("sensor ", sensorId, " added ", numSeeds, " seeds");
  }
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
  std::vector<bool> usedClusters;

  for (size_t istep = 0; istep < m_steps.size(); ++istep) {
    const auto& curr = m_steps[istep];
    auto& sensorEvent = event.getSensorEvent(curr.sensorId);

    // initialize usage mask, all clusters are assumed unused at the beginning
    usedClusters.assign(sensorEvent.numClusters(), false);

    // propagate states onto the current plane w/ material effects
    if (0 < istep) {
      const auto& prev = m_steps[istep - 1];
      propagateToCurrent(prev.processNoise, prev.plane, curr.plane, candidates);
    }

    // search for compatible clusters only on tracking planes.
    // by design, there are no candidates on the first one plane.
    if ((0 < istep) and (curr.useForTracking)) {
      // updates/extends candidates and sets clustersUsed flags
      searchSensor(m_d2LocMax, m_d2TimeMax, curr.sensorId, sensorEvent,
                   candidates, usedClusters);
      // ignore candidates that can never fullfill the final size cut
      removeShortCandidates(curr.candidateSizeMin, candidates);
    }

    // generate track candidates from unused clusters on seeding planes
    // this has to happen last so clusters are picked up first by existing
    // candidates generated on earlier seeding planes.
    if (curr.useForSeeding) {
      makeSeedsFromUnusedClusters(curr.sensorId, sensorEvent, curr.seedSlope,
                                  curr.seedSlopeCovariance, candidates,
                                  usedClusters);
    }
  }

  // final track selection and transformations
  removeBadCandidates(m_reducedChi2Max, candidates);
  sortCandidates(candidates);
  propagateToGlobal(m_steps.back().plane, candidates);
  addTracksToEvent(candidates, event);
}
