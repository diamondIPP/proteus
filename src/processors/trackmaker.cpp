#include "trackmaker.h"

#include <cassert>
#include <float.h>
#include <iostream>
#include <math.h>
#include <vector>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "processors/tracking.h"
#include "storage/event.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

using namespace Storage;

namespace Processors {

void Processors::TrackMaker::searchPlane(Track* track,
                                         std::vector<Track*>& candidates,
                                         unsigned int nplane) const
{
  assert(nplane < m_event->getNumPlanes() &&
         "TrackMaker: adding clusters in plane outside event range");
  assert(track && "TrackMaker: can't search plane with a void track");
  assert((int)nplane != m_maskedPlane && "TrackMaker: ouch");

  Plane* plane = m_event->getPlane(nplane);

  // Search over clusters in this event
  bool matchedCluster = false;
  for (unsigned int ncluster = 0; ncluster < plane->numClusters() + 1;
       ncluster++) {
    Track* trialTrack = 0;

    assert(track->getNumClusters() &&
           "TrackMaker: the track should have been seeded");
    Cluster* lastCluster = track->getCluster(track->getNumClusters() - 1);

    // Try to add the clusters to the track
    if (ncluster < plane->numClusters()) {
      Cluster* cluster = plane->getCluster(ncluster);
      if (cluster->getTrack())
        continue;

      const double errX = sqrt(pow(cluster->getPosErrX(), 2) +
                               pow(lastCluster->getPosErrX(), 2));
      const double errY = sqrt(pow(cluster->getPosErrY(), 2) +
                               pow(lastCluster->getPosErrY(), 2));

      // The real space distance between this cluster and the last
      const double distX = cluster->getPosX() - lastCluster->getPosX();
      const double distY = cluster->getPosY() - lastCluster->getPosY();
      const double distZ = cluster->getPosZ() - lastCluster->getPosZ();

      // Adjust the distance in X and Y to account for the slope, and normalize
      // in sigmas
      const double sigDistX = (distX - m_beamSlopeX * distZ) / errX;
      const double sigDistY = (distY - m_beamSlopeY * distZ) / errY;

      const double dist = sqrt(pow(sigDistX, 2) + pow(sigDistY, 2));

      if (dist > m_distMax)
        continue;
      // Found a good cluster, bifurcate the track and add the cluster
      matchedCluster = true;
      trialTrack = new Track(*track);
      trialTrack->addCluster(cluster);
    }
    // There were no more clusters in the track
    else if (!matchedCluster) {
      // No good clusters were found, use this track on the next plane
      trialTrack = track;
    } else {
      // At least one good cluster has been found
      delete track;
      continue; // This iteration of the loop isn't necessary
    }

    int planesRemaining = m_event->getNumPlanes() - nplane - 1;
    // Adjust for the masked plane if applicable
    if (m_maskedPlane >= 0 && m_maskedPlane > (int)nplane)
      planesRemaining -= 1;

    assert(planesRemaining >= 0 && "TrackMaker: something went terribly wrong");

    const int requiredClusters = m_nPointsMin - trialTrack->getNumClusters();

    // Check if it makes sense to continue building this track
    if (planesRemaining > 0 && requiredClusters <= planesRemaining) {
      const unsigned int nextPlane =
          (m_maskedPlane == (int)nplane + 1) ? nplane + 2 : nplane + 1;
      // This call will delete the trial track when it is done with it
      searchPlane(trialTrack, candidates, nextPlane);
    } else if (trialTrack->getNumClusters() < m_nPointsMin) {
      // This track can't continue and doesn't meet the cluster requirement
      delete trialTrack;
    } else {
      fitTrack(*trialTrack);
      candidates.push_back(trialTrack);
    }
  }
}

void Processors::TrackMaker::generateTracks(Event* event,
                                            double beamAngleX,
                                            double beamAngleY,
                                            int maskedPlane) const
{
  if (event->getNumPlanes() < 3)
    throw "TrackMaker: can't generate tracks from event with less than 3 "
          "planes";
  if (event->getNumTracks() > 0)
    throw "TrackMaker: tracks already exist for this event";

  m_beamSlopeX = beamAngleX;
  m_beamSlopeY = beamAngleY;

  if (maskedPlane >= (int)event->getNumPlanes() && VERBOSE) {
    cout << "WARNING :: TrackMaker: masked plane outside range";
    maskedPlane = -1;
  }

  m_event = event;
  m_maskedPlane = maskedPlane;

  // This is the number of planes available for tracking (after masking)
  const unsigned int numPlanes =
      (m_maskedPlane >= 0) ? m_event->getNumPlanes() - 1 : m_event->getNumPlanes();

  if (m_nPointsMin > numPlanes)
    throw "TrackMaker: min clusters exceeds number of planes";

  const unsigned int maxSeedPlanes = numPlanes - m_nPointsMin + 1;
  unsigned int numSeedPlanes = 0;
  if (m_numSeedPlanes > maxSeedPlanes) {
    numSeedPlanes =
        maxSeedPlanes; // Requested number of seed planes exceeds max
    if (VERBOSE)
      cout << "WARNING :: TrackMaker: too many seed planes, adjusting" << endl;
  } else {
    numSeedPlanes = m_numSeedPlanes; // Requested seed planes is OK
  }

  if (m_maskedPlane >= 0 && m_maskedPlane < (int)numSeedPlanes) {
    numSeedPlanes += 1; // Masking one of the seed planes, add one
  }

  if (numSeedPlanes < 1)
    throw "TrackMaker: can't make tracks with no seed planes";

  assert(numSeedPlanes < m_event->getNumPlanes() &&
         "TrackMaker: num seed planes is outside the plane range");

  for (unsigned int nplane = 0; nplane < numSeedPlanes; nplane++) {
    if ((int)nplane == m_maskedPlane)
      continue;

    Plane* plane = m_event->getPlane(nplane);

    // Each seed cluster generates a list of candidates from which the best is
    // kept
    for (unsigned int ncluster = 0; ncluster < plane->numClusters();
         ncluster++) {
      Cluster* cluster = plane->getCluster(ncluster);
      if (cluster->getTrack())
        continue;

      std::vector<Track*> candidates;
      TrackState state(cluster->getPosX(), cluster->getPosY());
      state.setCovOffset(cluster->covGlobal().Sub<SymMatrix2>(0,0));
      Track* seedTrack = new Track(state);
      seedTrack->addCluster(cluster);

      const unsigned int nextPlane =
          (maskedPlane == (int)nplane + 1) ? nplane + 2 : nplane + 1;
      searchPlane(seedTrack, candidates, nextPlane);

      const unsigned int numCandidates = candidates.size();
      if (!numCandidates)
        continue;

      Track* bestCandidate = 0;

      // If there is only one candidate, use it
      if (numCandidates == 1) {
        bestCandidate = candidates.at(0);
      }
      // Otherwise find the best candidate for this seed
      else {
        // Find the longest candidate size
        unsigned int mostClusters = 0;
        std::vector<Track*>::iterator it;
        for (it = candidates.begin(); it != candidates.end(); ++it) {
          Track* candidate = *(it);
          if (candidate->getNumClusters() > mostClusters)
            mostClusters = candidate->getNumClusters();
        }

        // Find the best chi2 amongst tracks which match the most clusters
        for (it = candidates.begin(); it != candidates.end(); ++it) {
          Track* candidate = *(it);
          if (candidate->getNumClusters() < mostClusters)
            continue;
          if (!bestCandidate || candidate->getChi2() > bestCandidate->getChi2())
            bestCandidate = candidate;
        }

        // Delete the rest of the candidates
        for (it = candidates.begin(); it != candidates.end(); ++it) {
          Track* candidate = *(it);
          if (candidate != bestCandidate)
            delete candidate;
        }
      }

      assert(bestCandidate && "TrackMaker: failed to select a candidate");

      // Finalize the best candidate
      m_event->addTrack(*bestCandidate)->fixClusterAssociation();
      delete bestCandidate;
      // for (unsigned int i = 0; i < bestCandidate->getNumClusters(); i++)
      //   bestCandidate->getCluster(i)->setTrack(bestCandidate);
    }
  }
}

Processors::TrackMaker::TrackMaker(double maxClusterDist,
                                   unsigned int numSeedPlanes,
                                   unsigned int minClusters,
                                   bool calcIntercepts)
    : m_distMax(maxClusterDist)
    , m_numSeedPlanes(numSeedPlanes)
    , m_nPointsMin(minClusters)
    , m_event(0)
    , m_maskedPlane(-1)
    , m_calcIntercepts(calcIntercepts)
{
  if (minClusters < 3)
    throw "TrackMaker: min clusters needs to be at least 3";
  if (numSeedPlanes < 1)
    throw "TrackMaker: needs at least one seed plane";
}

std::string Processors::TrackMaker::name() const { return "TrackMaker"; }

void Processors::TrackMaker::process(Storage::Event& event) const
{
  generateTracks(&event);
}
