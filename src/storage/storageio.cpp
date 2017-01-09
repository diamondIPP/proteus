#include "storageio.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#include "loopers/noisescan.h"
#include "mechanics/device.h"
#include "mechanics/sensor.h"
#include "storage/cluster.h"
#include "storage/event.h"
#include "storage/hit.h"
#include "storage/plane.h"
#include "storage/track.h"
#include "storage/trackstate.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(StorageIO)

void Storage::StorageIO::openRead(const std::string& path,
                                  const std::vector<bool>* planeMask)
{
  _file = TFile::Open(path.c_str(), "READ");
  if (!_file)
    throw std::runtime_error("Could not open file '" + path + "' for reading.");

  if (_numPlanes)
    INFO("disregarding specified number of planes");

  _numPlanes = 0; // Determine num planes from file structure

  unsigned int planeCount = 0;
  while (true) {
    std::string name("Plane" + std::to_string(planeCount));

    // Try to get this plane's directory
    TDirectory* dir = 0;
    _file->GetObject(name.c_str(), dir);
    if (!dir)
      break;

    planeCount++;

    if (planeMask && planeCount > planeMask->size())
      throw std::runtime_error("StorageIO: plane mask is too small");

    if (planeMask && planeMask->at(planeCount - 1))
      continue;

    _numPlanes++;

    TTree* hits;
    dir->GetObject("Hits", hits);
    _hits.push_back(hits);
    if (hits) {
      hits->SetBranchAddress("NHits", &numHits, &bNumHits);
      hits->SetBranchAddress("PixX", hitPixX, &bHitPixX);
      hits->SetBranchAddress("PixY", hitPixY, &bHitPixY);
      hits->SetBranchAddress("Timing", hitTiming, &bHitTiming);
      hits->SetBranchAddress("Value", hitValue, &bHitValue);
      hits->SetBranchAddress("HitInCluster", hitInCluster, &bHitInCluster);
    }

    TTree* clusters;
    dir->GetObject("Clusters", clusters);
    _clusters.push_back(clusters);
    if (clusters) {
      clusters->SetBranchAddress("NClusters", &numClusters, &bNumClusters);
      clusters->SetBranchAddress("Col", clusterPixX, &bClusterPixX);
      clusters->SetBranchAddress("Row", clusterPixY, &bClusterPixY);
      clusters->SetBranchAddress("StdCol", clusterPixErrX, &bClusterPixErrX);
      clusters->SetBranchAddress("StdRow", clusterPixErrY, &bClusterPixErrY);
      clusters->SetBranchAddress("Track", clusterInTrack, &bClusterInTrack);
    }

    TTree* intercepts;
    dir->GetObject("Intercepts", intercepts);
    _intercepts.push_back(intercepts);
    if (intercepts) {
      intercepts->SetBranchAddress("NIntercepts", &numIntercepts,
                                   &bNumIntercepts);
      intercepts->SetBranchAddress("U", interceptU, &bInterceptU);
      intercepts->SetBranchAddress("V", interceptV, &bInterceptV);
      intercepts->SetBranchAddress("SlopeU", interceptSlopeU,
                                   &bInterceptSlopeU);
      intercepts->SetBranchAddress("SlopeV", interceptSlopeV,
                                   &bInterceptSlopeV);
      intercepts->SetBranchAddress("StdU", interceptStdU, &bInterceptStdU);
      intercepts->SetBranchAddress("StdV", interceptStdV, &bInterceptStdV);
      intercepts->SetBranchAddress("StdSlopeU", interceptStdSlopeU,
                                   &bInterceptStdSlopeU);
      intercepts->SetBranchAddress("StdSlopeV", interceptStdSlopeV,
                                   &bInterceptStdSlopeV);
      intercepts->SetBranchAddress("Track", interceptTrack, &bInterceptTrack);
    }
  }

  _file->GetObject("Event", _eventInfo);
  if (_eventInfo) {
    _eventInfo->SetBranchAddress("TimeStamp", &timestamp, &bTimeStamp);
    _eventInfo->SetBranchAddress("FrameNumber", &frameNumber, &bFrameNumber);
    _eventInfo->SetBranchAddress("TriggerOffset", &triggerOffset,
                                 &bTriggerOffset);
    _eventInfo->SetBranchAddress("TriggerInfo", &triggerInfo, &bTriggerInfo);
    _eventInfo->SetBranchAddress("TriggerPhase", &triggerPhase, &bTriggerPhase);
    _eventInfo->SetBranchAddress("Invalid", &invalid, &bInvalid);
  }

  _file->GetObject("Tracks", _tracks);
  if (_tracks) {
    _tracks->SetBranchAddress("NTracks", &numTracks, &bNumTracks);
    _tracks->SetBranchAddress("SlopeX", trackSlopeX, &bTrackSlopeX);
    _tracks->SetBranchAddress("SlopeY", trackSlopeY, &bTrackSlopeY);
    _tracks->SetBranchAddress("SlopeErrX", trackSlopeErrX, &bTrackSlopeErrX);
    _tracks->SetBranchAddress("SlopeErrY", trackSlopeErrY, &bTrackSlopeErrY);
    _tracks->SetBranchAddress("OriginX", trackOriginX, &bTrackOriginX);
    _tracks->SetBranchAddress("OriginY", trackOriginY, &bTrackOriginY);
    _tracks->SetBranchAddress("OriginErrX", trackOriginErrX, &bTrackOriginErrX);
    _tracks->SetBranchAddress("OriginErrY", trackOriginErrY, &bTrackOriginErrY);
    _tracks->SetBranchAddress("CovarianceX", trackCovarianceX,
                              &bTrackCovarianceX);
    _tracks->SetBranchAddress("CovarianceY", trackCovarianceY,
                              &bTrackCovarianceY);
    _tracks->SetBranchAddress("Chi2", trackChi2, &bTrackChi2);
  }
}

void Storage::StorageIO::openTruncate(const std::string& path)
{
  _file = TFile::Open(path.c_str(), "RECREATE");
  if (!_file)
    throw std::runtime_error("Could not open file '" + path + "' for writing.");

  TDirectory* dir = 0;
  for (unsigned int nplane = 0; nplane < _numPlanes; nplane++) {
    std::string name("Plane" + std::to_string(nplane));

    dir = _file->mkdir(name.c_str());
    dir->cd();

    // Hits tree
    TTree* hits = new TTree("Hits", "Hits");
    _hits.push_back(hits);
    hits->Branch("NHits", &numHits, "NHits/I");
    hits->Branch("PixX", hitPixX, "HitPixX[NHits]/I");
    hits->Branch("PixY", hitPixY, "HitPixY[NHits]/I");
    hits->Branch("Timing", hitTiming, "HitTiming[NHits]/I");
    hits->Branch("Value", hitValue, "HitValue[NHits]/I");
    hits->Branch("HitInCluster", hitInCluster, "HitInCluster[NHits]/I");

    // Clusters tree
    TTree* clusters = new TTree("Clusters", "Clusters");
    _clusters.push_back(clusters);
    clusters->Branch("NClusters", &numClusters, "NClusters/I");
    clusters->Branch("Col", clusterPixX, "Col[NClusters]/D");
    clusters->Branch("Row", clusterPixY, "Row[NClusters]/D");
    clusters->Branch("StdCol", clusterPixErrX, "StdCol[NClusters]/D");
    clusters->Branch("StdRow", clusterPixErrY, "StdRow[NClusters]/D");
    clusters->Branch("Track", clusterInTrack, "Track[NClusters]/I");

    // Local track state tree
    TTree* intercepts = new TTree("Intercepts", "Intercepts");
    _intercepts.push_back(intercepts);
    intercepts->Branch("NIntercepts", &numIntercepts, "NIntercepts/I");
    intercepts->Branch("U", interceptU, "U[NIntercepts]/D");
    intercepts->Branch("V", interceptU, "V[NIntercepts]/D");
    intercepts->Branch("SlopeU", interceptSlopeU, "SlopeU[NIntercepts]/D");
    intercepts->Branch("SlopeV", interceptSlopeV, "SlopeV[NIntercepts]/D");
    intercepts->Branch("StdU", interceptStdU, "StdU[NIntercepts]/D");
    intercepts->Branch("StdV", interceptStdV, "StdV[NIntercepts]/D");
    intercepts->Branch("StdSlopeU", interceptStdSlopeU,
                       "StdSlopeU[NIntercepts]/D");
    intercepts->Branch("StdSlopeV", interceptStdSlopeV,
                       "StdSlopeV[NIntercepts]/D");
    intercepts->Branch("Track", interceptTrack, "Track[NIntercepts]/I");
  }

  _file->cd();

  // EventInfo tree
  _eventInfo = new TTree("Event", "Event information");
  _eventInfo->Branch("TimeStamp", &timestamp, "TimeStamp/l");
  _eventInfo->Branch("FrameNumber", &frameNumber, "FrameNumber/l");
  _eventInfo->Branch("TriggerOffset", &triggerOffset, "TriggerOffset/I");
  _eventInfo->Branch("TriggerInfo", &triggerInfo, "TriggerInfo/I");
  _eventInfo->Branch("TriggerPhase", &triggerPhase, "TriggerPhase/I");
  _eventInfo->Branch("Invalid", &invalid, "Invalid/O");

  // Tracks tree
  _tracks = new TTree("Tracks", "Track parameters");
  _tracks->Branch("NTracks", &numTracks, "NTracks/I");
  _tracks->Branch("SlopeX", trackSlopeX, "TrackSlopeX[NTracks]/D");
  _tracks->Branch("SlopeY", trackSlopeY, "TrackSlopeY[NTracks]/D");
  _tracks->Branch("SlopeErrX", trackSlopeErrX, "TrackSlopeErrX[NTracks]/D");
  _tracks->Branch("SlopeErrY", trackSlopeErrY, "TrackSlopeErrY[NTracks]/D");
  _tracks->Branch("OriginX", trackOriginX, "TrackOriginX[NTracks]/D");
  _tracks->Branch("OriginY", trackOriginY, "TrackOriginY[NTracks]/D");
  _tracks->Branch("OriginErrX", trackOriginErrX, "TrackOriginErrX[NTracks]/D");
  _tracks->Branch("OriginErrY", trackOriginErrY, "TrackOriginErrY[NTracks]/D");
  _tracks->Branch("CovarianceX", trackCovarianceX,
                  "TrackCovarianceX[NTracks]/D");
  _tracks->Branch("CovarianceY", trackCovarianceY,
                  "TrackCovarianceY[NTracks]/D");
  _tracks->Branch("Chi2", trackChi2, "TrackChi2[NTracks]/D");
}

namespace Storage {

//=========================================================
StorageIO::StorageIO(const std::string& filePath,
                     Mode fileMode,
                     unsigned int numPlanes,
                     const unsigned int treeMask,
                     const std::vector<bool>* planeMask)
    : _file(NULL)
    , _fileMode(fileMode)
    , _numPlanes(numPlanes)
    , _numEvents(0)
    , _tracks(0)
    , _eventInfo(0)
{

  // Plane mask holds a true for masked planes
  if (planeMask && fileMode == OUTPUT)
    throw std::runtime_error(
        "StorageIO: can't use a plane mask in output mode");

  clearVariables();

  if (fileMode == INPUT) {
    openRead(filePath, planeMask);
  } else {
    openTruncate(filePath);
  }

  // debug info
  INFO("file path: ", _file->GetPath());
  INFO("file mode: ", (_fileMode ? "OUTPUT" : "INPUT"));
  INFO("planes: ", _numPlanes);
  INFO("tree mask: ", treeMask);

  if (_numPlanes < 1)
    throw std::runtime_error("StorageIO: didn't initialize any planes");

  // Delete trees as per the tree flags
  if (treeMask) {
    for (unsigned int nplane = 0; nplane < _numPlanes; nplane++) {
      if (treeMask & Flags::HITS) {
        delete _hits.at(nplane);
        _hits.at(nplane) = NULL;
      }
      if (treeMask & Flags::CLUSTERS) {
        delete _clusters.at(nplane);
        _clusters.at(nplane) = NULL;
      }
    }
    if (treeMask & Flags::TRACKS) {
      delete _tracks;
      _tracks = NULL;
    }
    if (treeMask & Flags::EVENTINFO) {
      delete _eventInfo;
      _eventInfo = NULL;
    }
  }

  assert(_hits.size() == _clusters.size() &&
         "StorageIO: varying number of planes");

  _numEvents = 0;
  if (_fileMode == INPUT) { // INPUT mode
    Long64_t nEventInfo = (_eventInfo) ? _eventInfo->GetEntriesFast() : 0;
    Long64_t nTracks = (_tracks) ? _tracks->GetEntriesFast() : 0;
    Long64_t nHits = 0;
    Long64_t nClusters = 0;
    for (unsigned int nplane = 0; nplane < _numPlanes; nplane++) {
      if (_hits.at(nplane))
        nHits += _hits.at(nplane)->GetEntriesFast();
      if (_clusters.at(nplane))
        nClusters += _clusters.at(nplane)->GetEntriesFast();
    }

    if (nHits % _numPlanes || nClusters % _numPlanes)
      throw std::runtime_error(
          "StorageIO: number of events in different planes mismatch");

    nHits /= _numPlanes;
    nClusters /= _numPlanes;

    if (!_numEvents && nEventInfo)
      _numEvents = nEventInfo;
    if (!_numEvents && nTracks)
      _numEvents = nTracks;
    if (!_numEvents && nHits)
      _numEvents = nHits;
    if (!_numEvents && nClusters)
      _numEvents = nClusters;

    if ((nEventInfo && _numEvents != nEventInfo) ||
        (nTracks && _numEvents != nTracks) || (nHits && _numEvents != nHits) ||
        (nClusters && _numEvents != nClusters))
      throw std::runtime_error(
          "StorageIO: all trees don't have the same number of events");

  } // end if (_fileMode == INPUT)
} // end of StorageIO constructor

//=========================================================
StorageIO::~StorageIO()
{
  if (_file && _fileMode == OUTPUT) {
    INFO("file path: ", _file->GetPath());
    INFO("file mode: ", (_fileMode ? "OUTPUT" : "INPUT"));
    INFO("planes: ", _numPlanes);
    _file->Write();
    delete _file;
  }
}

//=========================================================
void StorageIO::clearVariables()
{
  timestamp = 0;
  frameNumber = 0;
  triggerOffset = 0;
  invalid = false;

  numHits = 0;
  for (int i = 0; i < MAX_HITS; i++) {
    hitPixX[i] = 0;
    hitPixY[i] = 0;
    hitTiming[i] = 0;
    hitValue[i] = 0;
    hitInCluster[i] = 0;
  }

  numClusters = 0;
  for (int i = 0; i < MAX_HITS; i++) {
    clusterPixX[i] = 0;
    clusterPixY[i] = 0;
    clusterPixErrX[i] = 0;
    clusterPixErrY[i] = 0;
    clusterInTrack[i] = 0;
  }

  numIntercepts = 0;
  for (int i = 0; i < MAX_HITS; ++i) {
    interceptTrack[i] = 0;
    interceptU[i] = 0;
    interceptV[i] = 0;
    interceptSlopeU[i] = 0;
    interceptSlopeV[i] = 0;
    interceptStdU[i] = 0;
    interceptStdV[i] = 0;
    interceptStdSlopeU[i] = 0;
    interceptStdSlopeV[i] = 0;
  }

  numTracks = 0;
  for (int i = 0; i < MAX_HITS; i++) {
    trackSlopeX[i] = 0;
    trackSlopeY[i] = 0;
    trackSlopeErrX[i] = 0;
    trackSlopeErrY[i] = 0;
    trackOriginX[i] = 0;
    trackOriginY[i] = 0;
    trackOriginErrX[i] = 0;
    trackOriginErrY[i] = 0;
    trackCovarianceX[i] = 0;
    trackCovarianceY[i] = 0;
    trackChi2[i] = 0;
  }
}

//=========================================================
void StorageIO::readEvent(uint64_t n, Event* event)
{
  /* Note: fill in reversed order: tracks first, hits last. This is so that
   * once a hit is produced, it can immediately recieve the address of its
   * parent cluster, likewise for clusters and track. */

  if (n >= _numEvents)
    throw std::runtime_error("StorageIO: requested event outside range");

  if (_eventInfo && _eventInfo->GetEntry(n) <= 0)
    throw std::runtime_error("StorageIO: error reading event tree");
  if (_tracks && _tracks->GetEntry(n) <= 0)
    throw std::runtime_error("StorageIO: error reading tracks tree");

  event->clear();
  event->setId(n);
  event->setTimestamp(timestamp);
  event->setFrameNumber(frameNumber);
  event->setTriggerOffset(triggerOffset);
  event->setTriggerInfo(triggerInfo);
  event->setTriggerPhase(triggerPhase);
  event->setInvalid(invalid);

  // Generate a list of track objects
  for (int ntrack = 0; ntrack < numTracks; ntrack++) {
    TrackState state(trackOriginX[ntrack], trackOriginY[ntrack],
                     trackSlopeX[ntrack], trackSlopeY[ntrack]);
    state.setErrU(trackOriginErrX[ntrack], trackSlopeErrX[ntrack],
                  trackCovarianceX[ntrack]);
    state.setErrV(trackOriginErrY[ntrack], trackSlopeErrY[ntrack],
                  trackCovarianceY[ntrack]);
    std::unique_ptr<Track> track(new Track(state));
    track->setGoodnessOfFit(trackChi2[ntrack]);
    event->addTrack(std::move(track));
  }

  for (unsigned int nplane = 0; nplane < _numPlanes; nplane++) {
    Storage::Plane* plane = event->getPlane(nplane);

    if (_hits.at(nplane) && (_hits[nplane]->GetEntry(n) <= 0))
      throw std::runtime_error("StorageIO: error reading hits tree");
    if (_clusters.at(nplane) && (_clusters[nplane]->GetEntry(n) <= 0))
      throw std::runtime_error("StorageIO: error reading clusters tree");
    if (_intercepts.at(nplane) && (_intercepts[nplane]->GetEntry(n) <= 0))
      throw std::runtime_error("StorageIO: error reading intercepts tree");

    // Add local track states
    for (Int_t iintercept = 0; iintercept < numIntercepts; ++iintercept) {
      Storage::TrackState local(interceptU[iintercept], interceptV[iintercept],
                                interceptSlopeU[iintercept],
                                interceptSlopeV[iintercept]);
      local.setErrU(interceptStdU[iintercept], interceptStdSlopeU[iintercept],
                    0);
      local.setErrV(interceptStdV[iintercept], interceptStdSlopeV[iintercept],
                    0);
      local.setTrack(event->getTrack(interceptTrack[iintercept]));
      plane->addState(std::move(local));
    }

    // Generate the cluster objects
    for (int ncluster = 0; ncluster < numClusters; ncluster++) {
      Cluster* cluster = plane->newCluster();
      cluster->setPixel(clusterPixX[ncluster], clusterPixY[ncluster],
                        clusterPixErrX[ncluster], clusterPixErrY[ncluster]);

      // If this cluster is in a track, mark this (and the tracks tree is
      // active)
      if (_tracks && (0 <= clusterInTrack[ncluster])) {
        Track* track = event->getTrack(clusterInTrack[ncluster]);
        track->addCluster(cluster);
        cluster->setTrack(track);
      }
    }

    // Generate a list of all hit objects
    for (int nhit = 0; nhit < numHits; nhit++) {
      Hit* hit = plane->newHit();
      hit->setAddress(hitPixX[nhit], hitPixY[nhit]);
      hit->setTime(hitTiming[nhit]);
      hit->setValue(hitValue[nhit]);

      // If this hit is in a cluster, mark this (and the clusters tree is
      // active)
      if (_clusters.at(nplane) && hitInCluster[nhit] >= 0) {
        Cluster* cluster = plane->getCluster(hitInCluster[nhit]);
        cluster->addHit(hit);
      }
    }
  } // end loop in planes
}

Event* StorageIO::readEvent(uint64_t index)
{
  Event* event = new Event(_numPlanes);
  readEvent(index, event);
  return event;
}

//=========================================================
void StorageIO::writeEvent(Event* event)
{

  if (_fileMode == INPUT)
    throw std::runtime_error("StorageIO: can't write event in input mode");

  timestamp = event->getTimeStamp();
  frameNumber = event->getFrameNumber();
  triggerOffset = event->getTriggerOffset();
  triggerInfo = event->getTriggerInfo();
  triggerPhase = event->getTriggerPhase();
  // cout<<triggerPhase<<endl;
  invalid = event->getInvalid();

  numTracks = event->getNumTracks();
  if (numTracks > MAX_TRACKS)
    throw std::runtime_error("StorageIO: event exceeds MAX_TRACKS");

  // Set the object track values into the arrays for writing to the root file
  for (int ntrack = 0; ntrack < numTracks; ntrack++) {
    Track* track = event->getTrack(ntrack);
    trackOriginX[ntrack] = track->getOriginX();
    trackOriginY[ntrack] = track->getOriginY();
    trackOriginErrX[ntrack] = track->getOriginErrX();
    trackOriginErrY[ntrack] = track->getOriginErrY();
    trackSlopeX[ntrack] = track->getSlopeX();
    trackSlopeY[ntrack] = track->getSlopeY();
    trackSlopeErrX[ntrack] = track->getSlopeErrX();
    trackSlopeErrY[ntrack] = track->getSlopeErrY();
    trackCovarianceX[ntrack] = track->getCovarianceX();
    trackCovarianceY[ntrack] = track->getCovarianceY();
    trackChi2[ntrack] = track->getChi2();
  }

  for (unsigned int nplane = 0; nplane < _numPlanes; nplane++) {
    Plane* plane = event->getPlane(nplane);

    // fill local states
    for (Index istate = 0; istate < plane->numStates(); ++istate) {
      const TrackState& local = plane->getState(istate);
      interceptTrack[istate] = local.track()->index();
      interceptU[istate] = local.offset().x();
      interceptV[istate] = local.offset().y();
      interceptSlopeU[istate] = local.slope().x();
      interceptSlopeV[istate] = local.slope().y();
      interceptStdU[istate] = local.stdOffsetU();
      interceptStdV[istate] = local.stdOffsetV();
      interceptStdSlopeU[istate] = local.stdSlopeU();
      interceptStdSlopeV[istate] = local.stdSlopeV();
    }

    numClusters = plane->numClusters();
    if (numClusters > MAX_CLUSTERS)
      throw std::runtime_error("StorageIO: event exceeds MAX_CLUSTERS");

    // Set the object cluster values into the arrays for writig into the root
    // file
    for (int ncluster = 0; ncluster < numClusters; ncluster++) {
      Cluster* cluster = plane->getCluster(ncluster);
      clusterPixX[ncluster] = cluster->getPixX();
      clusterPixY[ncluster] = cluster->getPixY();
      clusterPixErrX[ncluster] = cluster->getPixErrX();
      clusterPixErrY[ncluster] = cluster->getPixErrY();
      clusterInTrack[ncluster] =
          cluster->getTrack() ? cluster->getTrack()->getIndex() : -1;
    }

    numHits = plane->numHits();
    if (numHits > MAX_HITS)
      throw std::runtime_error("StorageIO: event exceeds MAX_HITS");

    // Set the object hit values into the arrays for writing into the root
    // file
    for (int nhit = 0; nhit < numHits; nhit++) {
      Hit* hit = plane->getHit(nhit);
      hitPixX[nhit] = hit->digitalCol();
      hitPixY[nhit] = hit->digitalRow();
      hitValue[nhit] = hit->value();
      hitTiming[nhit] = hit->time();
      hitInCluster[nhit] = hit->cluster() ? hit->cluster()->index() : -1;
    }

    if (nplane >= _hits.size())
      throw std::runtime_error(
          "StorageIO: event has too many planes for the storage");

    // Fill the plane by plane trees for this plane
    if (_hits.at(nplane))
      _hits.at(nplane)->Fill();
    if (_clusters.at(nplane))
      _clusters.at(nplane)->Fill();
    if (_intercepts.at(nplane))
      _intercepts.at(nplane)->Fill();

  } // end of nplane loop

  // Write the track and event info here so that if any errors occured they
  // won't be desynchronized
  if (_tracks)
    _tracks->Fill();
  if (_eventInfo)
    _eventInfo->Fill();

  _numEvents++;

} // end of writeEvent

} // end of namespace
