#include "storageio.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

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
      clusters->SetBranchAddress("Col", clusterCol, &bClusterCol);
      clusters->SetBranchAddress("Row", clusterRow, &bClusterRow);
      clusters->SetBranchAddress("VarCol", clusterVarCol, &bClusterVarCol);
      clusters->SetBranchAddress("VarRow", clusterVarRow, &bClusterVarRow);
      clusters->SetBranchAddress("CovColRow", clusterCovColRow,
                                 &bClusterCovColRow);
      clusters->SetBranchAddress("Track", clusterTrack, &bClusterTrack);
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
      intercepts->SetBranchAddress("Cov", interceptCov, &bInterceptCov);
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
    _tracks->SetBranchAddress("Chi2", trackChi2, &bTrackChi2);
    _tracks->SetBranchAddress("Dof", trackDof, &bTrackDof);
    _tracks->SetBranchAddress("X", trackX, &bTrackX);
    _tracks->SetBranchAddress("Y", trackY, &bTrackY);
    _tracks->SetBranchAddress("SlopeX", trackSlopeX, &bTrackSlopeX);
    _tracks->SetBranchAddress("SlopeY", trackSlopeY, &bTrackSlopeY);
    _tracks->SetBranchAddress("Cov", trackCov, &bTrackCov);
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
    clusters->Branch("Col", clusterCol, "Col[NClusters]/D");
    clusters->Branch("Row", clusterRow, "Row[NClusters]/D");
    clusters->Branch("VarCol", clusterVarCol, "VarCol[NClusters]/D");
    clusters->Branch("VarRow", clusterVarRow, "VarRow[NClusters]/D");
    clusters->Branch("CovColRow", clusterCovColRow, "CovColRow[NClusters]/D");
    clusters->Branch("Track", clusterTrack, "Track[NClusters]/I");

    // Local track state tree
    TTree* intercepts = new TTree("Intercepts", "Intercepts");
    _intercepts.push_back(intercepts);
    intercepts->Branch("NIntercepts", &numIntercepts, "NIntercepts/I");
    intercepts->Branch("U", interceptU, "U[NIntercepts]/D");
    intercepts->Branch("V", interceptU, "V[NIntercepts]/D");
    intercepts->Branch("SlopeU", interceptSlopeU, "SlopeU[NIntercepts]/D");
    intercepts->Branch("SlopeV", interceptSlopeV, "SlopeV[NIntercepts]/D");
    intercepts->Branch("Cov", interceptCov, "Cov[NIntercepts][10]/D");
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
  _tracks->Branch("Chi2", trackChi2, "Chi2[NTracks]/D");
  _tracks->Branch("Dof", trackDof, "Dof[NTracks]/I");
  _tracks->Branch("X", trackX, "X[NTracks]/D");
  _tracks->Branch("Y", trackY, "Y[NTracks]/D");
  _tracks->Branch("SlopeX", trackSlopeX, "SlopeX[NTracks]/D");
  _tracks->Branch("SlopeY", trackSlopeY, "SlopeY[NTracks]/D");
  _tracks->Branch("Cov", trackCov, "Cov[NTracks][10]/D");
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
    hitInCluster[i] = -1;
  }

  numClusters = 0;
  for (int i = 0; i < MAX_HITS; i++) {
    clusterCol[i] = 0;
    clusterRow[i] = 0;
    clusterVarCol[i] = 0;
    clusterVarRow[i] = 0;
    clusterCovColRow[i] = 0;
    clusterTrack[i] = 0;
  }

  numIntercepts = 0;
  for (int i = 0; i < MAX_HITS; ++i) {
    interceptU[i] = 0;
    interceptV[i] = 0;
    interceptSlopeU[i] = 0;
    interceptSlopeV[i] = 0;
    interceptTrack[i] = -1;
  }

  numTracks = 0;
  for (int i = 0; i < MAX_HITS; i++) {
    trackChi2[i] = -1;
    trackDof[i] = -1;
    trackX[i] = 0;
    trackY[i] = 0;
    trackSlopeX[i] = 0;
    trackSlopeY[i] = 0;
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
    TrackState state(trackX[ntrack], trackY[ntrack], trackSlopeX[ntrack],
                     trackSlopeY[ntrack]);
    state.setCov(trackCov[ntrack]);
    std::unique_ptr<Track> track(new Track(state));
    track->setGoodnessOfFit(trackChi2[ntrack], trackDof[ntrack]);
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
      local.setCov(interceptCov[iintercept]);
      local.setTrack(event->getTrack(interceptTrack[iintercept]));
      plane->addState(std::move(local));
    }

    // Generate the cluster objects
    for (int ncluster = 0; ncluster < numClusters; ncluster++) {
      SymMatrix2 cov;
      cov(0, 0) = clusterVarCol[ncluster];
      cov(1, 1) = clusterVarRow[ncluster];
      cov(0, 1) = clusterCovColRow[ncluster];
      Cluster* cluster = plane->newCluster();
      cluster->setPixel(XYPoint(clusterCol[ncluster], clusterRow[ncluster]),
                        cov);

      // If this cluster is in a track, mark this (and the tracks tree is
      // active)
      if (_tracks && (0 <= clusterTrack[ncluster])) {
        Track* track = event->getTrack(clusterTrack[ncluster]);
        track->addCluster(cluster);
        cluster->setTrack(track);
      }
    }

    // Generate a list of all hit objects
    for (int nhit = 0; nhit < numHits; nhit++) {
      Hit* hit = plane->addHit(hitPixX[nhit], hitPixY[nhit], hitTiming[nhit],
                               hitValue[nhit]);

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

  timestamp = event->timestamp();
  frameNumber = event->frameNumber();
  triggerOffset = event->triggerOffset();
  triggerInfo = event->triggerInfo();
  triggerPhase = event->triggerPhase();
  // cout<<triggerPhase<<endl;
  invalid = event->invalid();

  numTracks = event->numTracks();
  if (numTracks > MAX_TRACKS)
    throw std::runtime_error("StorageIO: event exceeds MAX_TRACKS");

  // Set the object track values into the arrays for writing to the root file
  for (int ntrack = 0; ntrack < numTracks; ntrack++) {
    const Track& track = *event->getTrack(ntrack);
    trackChi2[ntrack] = track.chi2();
    trackDof[ntrack] = track.degreesOfFreedom();
    const TrackState& state = track.globalState();
    trackX[ntrack] = state.offset().x();
    trackY[ntrack] = state.offset().y();
    trackSlopeX[ntrack] = state.slope().x();
    trackSlopeY[ntrack] = state.slope().y();
    std::copy(state.cov().begin(), state.cov().end(), trackCov[ntrack]);
  }

  for (unsigned int nplane = 0; nplane < _numPlanes; nplane++) {
    Plane* plane = event->getPlane(nplane);

    // fill local states
    for (Index istate = 0; istate < plane->numStates(); ++istate) {
      const TrackState& local = plane->getState(istate);
      interceptU[istate] = local.offset().x();
      interceptV[istate] = local.offset().y();
      interceptSlopeU[istate] = local.slope().x();
      interceptSlopeV[istate] = local.slope().y();
      std::copy(local.cov().begin(), local.cov().end(), interceptCov[istate]);
      interceptTrack[istate] = local.track()->index();
    }

    numClusters = plane->numClusters();
    if (numClusters > MAX_CLUSTERS)
      throw std::runtime_error("StorageIO: event exceeds MAX_CLUSTERS");

    // Set the object cluster values into the arrays for writig into the root
    // file
    for (int ncluster = 0; ncluster < numClusters; ncluster++) {
      const Cluster& cluster = *plane->getCluster(ncluster);
      clusterCol[ncluster] = cluster.posPixel().x();
      clusterRow[ncluster] = cluster.posPixel().y();
      clusterVarCol[ncluster] = cluster.covPixel()(0, 0);
      clusterVarRow[ncluster] = cluster.covPixel()(1, 1);
      clusterCovColRow[ncluster] = cluster.covPixel()(0, 1);
      clusterTrack[ncluster] = cluster.track() ? cluster.track()->index() : -1;
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
