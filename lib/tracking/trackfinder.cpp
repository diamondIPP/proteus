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

using Storage::Cluster;
using Storage::Event;
using Storage::SensorEvent;
using Storage::Track;
using Storage::TrackState;

PT_SETUP_LOCAL_LOGGER(TrackFinder)

Tracking::TrackFinder::TrackFinder(const Mechanics::Device& device,
                                   const std::vector<Index>& sensors,
                                   const Index numClustersMin,
                                   const double searchSigmaMax,
                                   const double redChi2Max)
    : m_geo(device.geometry())
    , m_sensorIds(Mechanics::sortedAlongBeam(device.geometry(), sensors))
    , m_numClustersMin(numClustersMin)
    // 2-d Mahalanobis distance peaks at 2 and not at 1
    , m_d2Max((searchSigmaMax < 0) ? -1 : (2 * searchSigmaMax * searchSigmaMax))
    , m_redChi2Max(redChi2Max)
{
  if (m_sensorIds.size() < 2)
    throw std::runtime_error("Need at least two sensors two find tracks");
  if (m_sensorIds.size() < m_numClustersMin)
    throw std::runtime_error(
        "Number of tracking sensors < minimum number of clusters");
  // TODO 2016-11 msmk: check that sensor ids are unique
}

std::string Tracking::TrackFinder::name() const { return "TrackFinder"; }

// remove track candidates that are too short
static void removeShortTracks(size_t numPointsMin,
                              size_t numRemainingSensors,
                              std::vector<Track>& tracks)
{
  // a track is long enough if it can potentially still fullfill the
  // mininum number of points on track at the end of the search
  auto isShort = [=](const auto& track) {
    return (track.size() + numRemainingSensors) < numPointsMin;
  };
  auto rm = std::remove_if(tracks.begin(), tracks.end(), isShort);
  tracks.erase(rm, tracks.end());
}

// remove track badly fitted candidates
static void removeBadTracks(Scalar reducedChi2Max, std::vector<Track>& tracks)
{
  auto isBad = [=](const auto& track) {
    return reducedChi2Max <= track.reducedChi2();
  };
  auto rm = std::remove_if(tracks.begin(), tracks.end(), isBad);
  tracks.erase(rm, tracks.end());
}

void Tracking::TrackFinder::execute(Event& event) const
{
  std::vector<Track> candidates;

  // first iteration over all seed sensors
  size_t numSeedSeensors = 1 + (m_sensorIds.size() - m_numClustersMin);
  for (size_t i = 0; i < numSeedSeensors; ++i) {
    Index seedSensor = m_sensorIds[i];
    auto& seedEvent = event.getSensorEvent(seedSensor);

    // TODO 2017-05-10 msmk:
    // ideally we would find all candidates starting from all seed sensors first
    // and only then select the final tracks. this should avoid cases where
    // a bad 6-hit track, e.g. with one extra noise hit, shadows a good 5-hit
    // track with the same cluster content.
    // this would require a combined metric that sorts both by number of hits
    // and by track quality. defining such a combined metric is always a bit
    // arbitrary and introduces additional corner cases that i would prefer
    // to avoid.
    // just sorting by chi2 along results yields 50% 5-hit tracks in the
    // test samples; compared to 90% 6-hit tracks with the default algorithm.
    // a single misaligned sensor will always result in a better track with
    // less hits.
    candidates.clear();

    // generate track candidates from all unused clusters on the seed sensor
    for (Index icluster = 0; icluster < seedEvent.numClusters(); ++icluster) {
      auto& cluster = seedEvent.getCluster(icluster);
      if (cluster.isInTrack())
        continue;
      // abuse global state to store the state on the current plane
      TrackState seedState(cluster.position(), cluster.positionCov(),
                           m_geo.getBeamSlope(seedSensor),
                           m_geo.getBeamSlopeCovariance(seedSensor));
      // no fit yet -> no chi2, undefined degrees-of-freedom
      candidates.emplace_back(seedState, 0, -1);
      candidates.back().addCluster(seedSensor, icluster);
    }

    // second iteration over remaining sensors to find compatible points
    for (size_t j = i; (j + 1) < m_sensorIds.size(); ++j) {
      Index sourceId = m_sensorIds[j];
      Index targetId = m_sensorIds[j + 1];

      // propagate expected state onto the search sensor
      // TODO handle dead material, e.g. sensors not used for tracking
      const auto& source = m_geo.getPlane(sourceId);
      const auto& target = m_geo.getPlane(targetId);
      for (auto& track : candidates) {
        track.setGlobalState(propagateTo(track.globalState(), source, target));
      }

      // search for compatible points and update local state
      searchSensor(targetId, event.getSensorEvent(targetId), candidates);

      // ignore candidates that can never fullfill the final cut
      if (0 < m_numClustersMin) {
        size_t numRemainingSensors = m_sensorIds.size() - (j + 2);
        removeShortTracks(m_numClustersMin, numRemainingSensors, candidates);
      }
    }

    // remove bad tracks if requested
    if (0 < m_redChi2Max) {
      removeBadTracks(m_redChi2Max, candidates);
    }
    // transport last local state to the global plane
    const auto& source = m_geo.getPlane(m_sensorIds.back());
    const auto& target = Mechanics::Plane{}; // global plane
    for (auto& track : candidates) {
      track.setGlobalState(propagateTo(track.globalState(), source, target));
    }
    // store best, unique tracks in the event
    selectTracks(candidates, event);
  }
}

/** Search for matching clusters for all candidates on the given sensor.
 *
 * Ambiguities are not resolved but result in additional track candidates.
 */
void Tracking::TrackFinder::searchSensor(
    Storage::Event& event,
    Index iSearchSensor,
    std::vector<Storage::Track>& candidates) const
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
  // we are modifying the list of candidates while iterating over it. always
  // make sure that we access candidates only through the index and not via
  // a reference or an iterator. If the underlying memory gets reallocated
  // we will access random memory and break the heap.
  Index numTracks = static_cast<Index>(candidates.size());
  for (Index itrack = 0; itrack < numTracks; ++itrack) {
    TrackState state = candidates[itrack].globalState();
    Scalar chi2 = candidates[itrack].chi2();
    int numMatchedClusters = 0;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      auto& cluster = sensorEvent.getCluster(icluster);

      // clusters already in use must be ignored
      if (cluster.isInTrack()) {
        continue;
      }

      // compute kalman filter update from this cluster

      // predicted residuals and covariance
      Vector2 r = cluster.uv() - H * state.params();
      Matrix2 R = cluster.uvCov() + transformCovariance(H, state.cov());
      // Kalman gain matrix
      Matrix<Scalar, 6, 2> K = state.cov() * H.transpose() * R.inverse();
      // filtered local state and covariance
      // use numerical superior (but slower formula) for covariance update
      Vector6 xf = state.params() + K * r;
      SymMatrix6 Cf =
          transformCovariance(SymMatrix6::Identity() - K * H, state.cov()) +
          transformCovariance(K, cluster.uvCov());
      // filtered residuals and covariance
      r = cluster.uv() - H * xf;
      R = cluster.uvCov() - transformCovariance(H, Cf);
      // compute chi2 update
      Scalar chi2Update = mahalanobisSquared(R, r);

      // check if the cluster is compatible
      if ((0 < m_d2Max) && (m_d2Max < chi2Update)) {
        continue;
      }

      // if this is not the first cluster, we need to bifurcate the track
      auto& track =
          (numMatchedClusters == 0)
              ? candidates[itrack]
              : (candidates.push_back(candidates[itrack]), candidates.back());
      // this replaces a previous
      track.addCluster(sensorId, icluster);
      track.setGlobalState({xf, Cf});
      track.setGoodnessOfFit(chi2 + chi2Update, 2 * track.size() - 6);
      numMatchedClusters += 1;
    }
  }
}

/** Add best, unique tracks to the event. */
void Tracking::TrackFinder::selectTracks(std::vector<Track>& candidates,
                                         Event& event) const
{
  // sort good candidates first, i.e. longest track and smallest chi2
  std::sort(
      candidates.begin(), candidates.end(), [](const auto& ta, const auto& tb) {
        return (ta.size() > tb.size()) or (ta.reducedChi2() < tb.reducedChi2());
      });

  for (auto& track : candidates) {

    // all clusters must be unused
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
  }
}
