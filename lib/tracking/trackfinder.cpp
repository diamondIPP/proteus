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
                              std::vector<Track>& candidates)
{
  // long enough means the track can potentially still fullfill the
  // mininum number of points on track at the end of the search
  auto isLongEnough = [&](const auto& candidate) {
    return numPointsMin <= (candidate.size() + numRemainingSensors);
  };
  auto bad = std::partition(candidates.begin(), candidates.end(), isLongEnough);
  // remove the short tracks
  candidates.erase(bad, candidates.end());
}

// fit all tracks in the global system
static void fitGlobal(const Mechanics::Geometry& geo,
                      const Event& event,
                      std::vector<Track>& candidates)
{
  for (auto& track : candidates) {
    Tracking::LineFitter3D fitter;
    for (const auto& c : track.clusters()) {
      const auto& plane = geo.getPlane(c.sensor);
      const auto& cluster =
          event.getSensorEvent(c.sensor).getCluster(c.cluster);
      // convert to global system
      Vector4 global = plane.toGlobal(cluster.position());
      Vector4 weight =
          transformCovariance(plane.linearToGlobal(), cluster.positionCov())
              .diagonal()
              .cwiseInverse();
      fitter.addPoint(global, weight);
    }
    fitter.fit();
    track.setGoodnessOfFit(fitter.chi2(), fitter.dof());
    track.setGlobalState({fitter.params(), fitter.cov()});
  }
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
      for (auto& candidate : candidates) {
        candidate.setGlobalState(
            propagateTo(candidate.globalState(), source, target));
      }

      // search for compatible points and update local state
      searchSensor(targetId, event.getSensorEvent(targetId), candidates);

      // remove candidates that are already too short
      size_t numRemainingSensors = m_sensorIds.size() - (j + 2);
      removeShortTracks(m_numClustersMin, numRemainingSensors, candidates);
    }

    // get final chi2 and global parameters for all possible candidates
    fitGlobal(m_geo, event, candidates);

    // select final tracks after all possible candidates have been found
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
  const auto& target = m_geo.getPlane(iSearchSensor);
  auto& sensorEvent = event.getSensorEvent(iSearchSensor);
  auto beamSlope = m_geo.getBeamSlope(sensorId);
  auto beamSlopeCov = m_geo.getBeamSlopeCovariance(sensorId);

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
    int numMatchedClusters = 0;

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      auto& cluster = sensorEvent.getCluster(icluster);

      // clusters already in use must be ignored
      if (cluster.isInTrack()) {
        continue;
      }

      Vector2 delta(cluster.u() - state.loc0(), cluster.v() - state.loc1());
      SymMatrix2 cov = cluster.uvCov() + state.loc01Cov();
      auto d2 = mahalanobisSquared(cov, delta);

      // check if the cluster is compatible
      if ((0 < m_d2Max) && (m_d2Max < d2)) {
        continue;
      }
      // updated track state for further propagation
      TrackState updatedState(cluster.position(), cluster.positionCov(),
                              beamSlope, beamSlopeCov);

      if (numMatchedClusters == 0) {
        // first cluster matched to this tracks
        candidates[itrack].addCluster(sensorId, icluster);
        candidates[itrack].setGlobalState(updatedState);
      } else {
        // more than one match cluster/ matching ambiguity -> bifurcate track
        candidates.push_back(candidates[itrack]);
        // this replaces any previously added cluster for this sensor
        candidates.back().addCluster(sensorId, icluster);
        candidates.back().setGlobalState(updatedState);
      }
      numMatchedClusters += 1;
    }
  }
}

/** Add tracks selected by chi2 and unique cluster association to the event. */
void Tracking::TrackFinder::selectTracks(std::vector<Track>& candidates,
                                         Event& event) const
{
  // sort good candidates first, i.e. longest track and smallest chi2
  std::sort(
      candidates.begin(), candidates.end(), [](const auto& ta, const auto& tb) {
        return (ta.size() > tb.size()) or (ta.reducedChi2() < tb.reducedChi2());
      });

  // fix cluster assignment starting w/ best tracks first
  for (auto& track : candidates) {
    // apply track cuts; minimum number of points is ensured by iteration steps
    if ((0 < m_redChi2Max) && (m_redChi2Max < track.reducedChi2()))
      continue;

    // check that all constituent clusters are still unused
    bool hasUsedClusters = false;
    for (const auto& c : track.clusters()) {
      const auto& cluster =
          event.getSensorEvent(c.sensor).getCluster(c.cluster);
      if (cluster.isInTrack()) {
        hasUsedClusters = true;
        break;
      }
    }
    // some clusters are already used by other tracks
    if (hasUsedClusters)
      continue;

    // add new, good track to the event; also fixes cluster-track association
    event.addTrack(track);
  }
}
