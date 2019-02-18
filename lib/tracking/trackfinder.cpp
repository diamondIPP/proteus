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

void Tracking::TrackFinder::execute(Storage::Event& event) const
{
  std::vector<Storage::Track> candidates;

  // first iteration over all seed sensors
  size_t numSeedSeensors = 1 + (m_sensorIds.size() - m_numClustersMin);
  for (size_t i = 0; i < numSeedSeensors; ++i) {
    Index seedSensor = m_sensorIds[i];
    SensorEvent& seedEvent = event.getSensorEvent(seedSensor);

    // generate track candidates from all unused clusters on the seed sensor
    candidates.clear();
    for (Index icluster = 0; icluster < seedEvent.numClusters(); ++icluster) {
      Cluster& cluster = seedEvent.getCluster(icluster);
      if (cluster.isInTrack())
        continue;
      candidates.push_back(Track{});
      candidates.back().addCluster(seedSensor, icluster);
    }

    // second iteration over remaining sensors to find compatible points
    for (size_t j = i + 1; j < m_sensorIds.size(); ++j) {
      searchSensor(event, m_sensorIds[j], candidates);

      // remove seeds that are already too short
      size_t remainingSensors = m_sensorIds.size() - (j + 1);
      auto isLongEnough = [&](const Storage::Track& candidate) {
        return (m_numClustersMin <= (candidate.size() + remainingSensors));
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
    Storage::Event& event,
    Index iSearchSensor,
    std::vector<Storage::Track>& candidates) const
{
  const auto& target = m_geo.getPlane(iSearchSensor);
  auto& sensorEvent = event.getSensorEvent(iSearchSensor);

  // loop only over the initial candidates and not the added ones
  Index numTracks = static_cast<Index>(candidates.size());
  for (Index itrack = 0; itrack < numTracks; ++itrack) {
    Storage::Track& track = candidates[itrack];
    Index iLastSensor = track.clusters().back().sensor;
    Index iLastCluster = track.clusters().back().cluster;
    const Storage::Cluster& lastCluster =
        event.getSensorEvent(iLastSensor).getCluster(iLastCluster);
    Index matched = kInvalidIndex;

    // estimated track on the source plane using last cluster and beam
    Storage::TrackState onSource(lastCluster.position(),
                                 lastCluster.positionCov(),
                                 m_geo.getBeamSlope(iLastSensor),
                                 m_geo.getBeamSlopeCovariance(iLastSensor));
    // propagate track to the target plane
    Storage::TrackState onTarget =
        Tracking::propagateTo(onSource, m_geo.getPlane(iLastSensor), target);

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
      Storage::Cluster& curr = sensorEvent.getCluster(icluster);

      // clusters already in use must be ignored
      if (curr.isInTrack())
        continue;

      Vector2 delta(curr.u() - onTarget.loc0(), curr.v() - onTarget.loc1());
      SymMatrix2 cov = curr.uvCov() + onTarget.loc01Cov();
      auto d2 = mahalanobisSquared(cov, delta);

      if ((0 < m_d2Max) && (m_d2Max < d2))
        continue;

      if (matched == kInvalidIndex) {
        // first matching cluster
        matched = icluster;
      } else {
        // matching ambiguity -> bifurcate track
        candidates.push_back(track);
        candidates.back().addCluster(iSearchSensor, icluster);
      }
    }
    // first matched cluster can be only be added after all other clusters
    // have been considered. otherwise it would be already added to the
    // candidate when it bifurcates and the new candidate would have two
    // clusters on this sensor.
    if (matched != kInvalidIndex) {
      track.addCluster(iSearchSensor, matched);
    }
  }
}

// Fit track in the global system and store chi2 and global state.
static void fitGlobal(const Mechanics::Geometry& geo,
                      const Storage::Event& event,
                      Storage::Track& track)
{
  Tracking::LineFitter3D fitter;

  for (const auto& c : track.clusters()) {
    const auto& plane = geo.getPlane(c.sensor);
    const Storage::Cluster& cluster =
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
  track.setGlobalState(fitter.params(), fitter.cov());
  track.setGoodnessOfFit(fitter.chi2(), fitter.dof());
}

/** Add tracks selected by chi2 and unique cluster association to the event. */
void Tracking::TrackFinder::selectTracks(
    std::vector<Storage::Track>& candidates, Storage::Event& event) const
{
  // ensure chi2 value is up-to-date
  for (auto& candidate : candidates) {
    fitGlobal(m_geo, event, candidate);
  }
  // sort good candidates first, i.e. longest track and smallest chi2
  std::sort(
      candidates.begin(), candidates.end(), [](const auto& ta, const auto& tb) {
        return (ta.size() > tb.size()) or (ta.reducedChi2() < tb.reducedChi2());
      });

  // fix cluster assignment starting w/ best tracks first
  for (auto& track : candidates) {

    // apply track cuts
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
