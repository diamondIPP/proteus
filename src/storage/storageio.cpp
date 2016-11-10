#include "storageio.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#include "loopers/noisescan.h"
#include "mechanics/device.h"
#include "mechanics/noisemask.h"
#include "mechanics/sensor.h"
#include "storage/cluster.h"
#include "storage/event.h"
#include "storage/hit.h"
#include "storage/plane.h"
#include "storage/track.h"
#include "utils/logger.h"

using Utils::logger;

void Storage::StorageIO::openRead(const std::string& path, const std::vector<bool>* planeMask)
{
  _file = TFile::Open(path.c_str(), "READ");
  if (!_file)
    throw std::runtime_error("Could not open file '" + path + "' for reading.");

  if (_numPlanes)
    INFO("StorageIO: disregarding specified number of planes\n");

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
      hits->SetBranchAddress("Value", hitValue, &bHitValue);
      hits->SetBranchAddress("Timing", hitTiming, &bHitTiming);
      hits->SetBranchAddress("HitInCluster", hitInCluster, &bHitInCluster);
      hits->SetBranchAddress("PosX", hitPosX, &bHitPosX);
      hits->SetBranchAddress("PosY", hitPosY, &bHitPosY);
      hits->SetBranchAddress("PosZ", hitPosZ, &bHitPosZ);
    }

    TTree* clusters;
    dir->GetObject("Clusters", clusters);
    _clusters.push_back(clusters);
    if (clusters) {
      clusters->SetBranchAddress("NClusters", &numClusters, &bNumClusters);
      clusters->SetBranchAddress("PixX", clusterPixX, &bClusterPixX);
      clusters->SetBranchAddress("PixY", clusterPixY, &bClusterPixY);
      clusters->SetBranchAddress("PixErrX", clusterPixErrX, &bClusterPixErrX);
      clusters->SetBranchAddress("PixErrY", clusterPixErrY, &bClusterPixErrY);
      clusters->SetBranchAddress("InTrack", clusterInTrack, &bClusterInTrack);
      clusters->SetBranchAddress("PosX", clusterPosX, &bClusterPosX);
      clusters->SetBranchAddress("PosY", clusterPosY, &bClusterPosY);
      clusters->SetBranchAddress("PosZ", clusterPosZ, &bClusterPosZ);
      clusters->SetBranchAddress("PosErrX", clusterPosErrX, &bClusterPosErrX);
      clusters->SetBranchAddress("PosErrY", clusterPosErrY, &bClusterPosErrY);
      clusters->SetBranchAddress("PosErrZ", clusterPosErrZ, &bClusterPosErrZ);
    }

    TTree* intercepts;
    dir->GetObject("Intercepts", intercepts);
    _intercepts.push_back(intercepts);
    if (intercepts) {
      intercepts->SetBranchAddress("NIntercepts", &numIntercepts, &bNumIntercepts);
      intercepts->SetBranchAddress("Track", interceptTrack, &bInterceptTrack);
      intercepts->SetBranchAddress("U", interceptU, &bInterceptU);
      intercepts->SetBranchAddress("V", interceptV, &bInterceptV);
      intercepts->SetBranchAddress("SlopeU", interceptSlopeU, &bInterceptSlopeU);
      intercepts->SetBranchAddress("SlopeV", interceptSlopeV, &bInterceptSlopeV);
      intercepts->SetBranchAddress("StdU", interceptStdU, &bInterceptStdU);
      intercepts->SetBranchAddress("StdV", interceptStdV, &bInterceptStdV);
      intercepts->SetBranchAddress("StdSlopeU", interceptStdSlopeU, &bInterceptStdSlopeU);
      intercepts->SetBranchAddress("StdSlopeV", interceptStdSlopeV, &bInterceptStdSlopeV);
    }
  }

  _file->GetObject("Event", _eventInfo);
  if (_eventInfo) {
    _eventInfo->SetBranchAddress("TimeStamp", &timeStamp, &bTimeStamp);
    _eventInfo->SetBranchAddress("FrameNumber", &frameNumber, &bFrameNumber);
    _eventInfo->SetBranchAddress(
        "TriggerOffset", &triggerOffset, &bTriggerOffset);
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
    _tracks->SetBranchAddress(
        "CovarianceX", trackCovarianceX, &bTrackCovarianceX);
    _tracks->SetBranchAddress(
        "CovarianceY", trackCovarianceY, &bTrackCovarianceY);
    _tracks->SetBranchAddress("Chi2", trackChi2, &bTrackChi2);
  }

  _file->GetObject("SummaryTree", _summaryTree);
  if (_summaryTree) {
    _summaryTree->SetBranchAddress("NRuns", &st_numRuns, &b_NumRuns);
    _summaryTree->SetBranchAddress("Run", st_run, &b_Run);
    _summaryTree->SetBranchAddress(
        "nscan_NRuns", &st_nscan_numRuns, &b_NoiseScan_NumRuns);
    _summaryTree->SetBranchAddress("nscan_Run", st_nscan_run, &b_NoiseScan_Run);
    _summaryTree->SetBranchAddress(
        "nscan_maxFactor", &st_nscan_maxFactor, &b_NoiseScan_MaxFactor);
    _summaryTree->SetBranchAddress("nscan_maxOccupancy",
                                   &st_nscan_maxOccupancy,
                                   &b_NoiseScan_MaxOccupancy);
    _summaryTree->SetBranchAddress(
        "nscan_bottomX", &st_nscan_bottom_x, &b_NoiseScan_BottomX);
    _summaryTree->SetBranchAddress(
        "nscan_upperX", &st_nscan_upper_x, &b_NoiseScan_UpperX);
    _summaryTree->SetBranchAddress(
        "nscan_bottomY", &st_nscan_bottom_y, &b_NoiseScan_BottomY);
    _summaryTree->SetBranchAddress(
        "nscan_upperY", &st_nscan_upper_y, &b_NoiseScan_UpperY);
    _summaryTree->SetBranchAddress("nscan_NNoisyPixels",
                                   &st_nscan_numNoisyPixels,
                                   &b_NoiseScan_NumNoisyPixels);
    // ensure enough space is available for the maximum number of possible
    // noise pixels, i.e. #planes * #pixels_per_plane
    // use Mimosa26 (1152 cols x 576 rows) as upper limit
    size_t max_size = _numPlanes * 1152 * 576;
    st_nscan_noisyPixel_plane.resize(max_size);
    st_nscan_noisyPixel_x.resize(max_size);
    st_nscan_noisyPixel_y.resize(max_size);
    _summaryTree->SetBranchAddress("nscan_noisyPixel_plane",
                                   st_nscan_noisyPixel_plane.data(),
                                   &b_NoiseScan_NoisyPixelPlane);
    _summaryTree->SetBranchAddress("nscan_noisyPixel_x",
                                   st_nscan_noisyPixel_x.data(),
                                   &b_NoiseScan_NoisyPixelX);
    _summaryTree->SetBranchAddress("nscan_noisyPixel_y",
                                   st_nscan_noisyPixel_y.data(),
                                   &b_NoiseScan_NoisyPixelY);
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
    hits->Branch("Value", hitValue, "HitValue[NHits]/I");
    hits->Branch("Timing", hitTiming, "HitTiming[NHits]/I");
    hits->Branch("HitInCluster", hitInCluster, "HitInCluster[NHits]/I");
    hits->Branch("PosX", hitPosX, "HitPosX[NHits]/D");
    hits->Branch("PosY", hitPosY, "HitPosY[NHits]/D");
    hits->Branch("PosZ", hitPosZ, "HitPosZ[NHits]/D");

    // Clusters tree
    TTree* clusters = new TTree("Clusters", "Clusters");
    _clusters.push_back(clusters);
    clusters->Branch("NClusters", &numClusters, "NClusters/I");
    clusters->Branch("PixX", clusterPixX, "ClusterPixX[NClusters]/D");
    clusters->Branch("PixY", clusterPixY, "ClusterPixY[NClusters]/D");
    clusters->Branch("PixErrX", clusterPixErrX, "ClusterPixErrX[NClusters]/D");
    clusters->Branch("PixErrY", clusterPixErrY, "ClusterPixErrY[NClusters]/D");
    clusters->Branch("InTrack", clusterInTrack, "ClusterInTrack[NClusters]/I");
    clusters->Branch("PosX", clusterPosX, "ClusterPosX[NClusters]/D");
    clusters->Branch("PosY", clusterPosY, "ClusterPosY[NClusters]/D");
    clusters->Branch("PosZ", clusterPosZ, "ClusterPosZ[NClusters]/D");
    clusters->Branch("PosErrX", clusterPosErrX, "ClusterPosErrX[NClusters]/D");
    clusters->Branch("PosErrY", clusterPosErrY, "ClusterPosErrY[NClusters]/D");
    clusters->Branch("PosErrZ", clusterPosErrZ, "ClusterPosErrZ[NClusters]/D");

    // Local track state tree
    TTree* intercepts = new TTree("Intercepts", "Intercepts");
    _intercepts.push_back(intercepts);
    intercepts->Branch("NIntercepts", &numIntercepts, "NIntercepts/I");
    intercepts->Branch("Track", interceptTrack, "Track[NIntercepts]/I");
    intercepts->Branch("U", interceptU, "U[NIntercepts]/D");
    intercepts->Branch("V", interceptU, "V[NIntercepts]/D");
    intercepts->Branch("SlopeU", interceptSlopeU, "SlopeU[NIntercepts]/D");
    intercepts->Branch("SlopeV", interceptSlopeV, "SlopeV[NIntercepts]/D");
    intercepts->Branch("StdU", interceptStdU, "StdU[NIntercepts]/D");
    intercepts->Branch("StdV", interceptStdV, "StdV[NIntercepts]/D");
    intercepts->Branch("StdSlopeU", interceptStdSlopeU, "StdSlopeU[NIntercepts]/D");
    intercepts->Branch("StdSlopeV", interceptStdSlopeV, "StdSlopeV[NIntercepts]/D");
  }

  _file->cd();

  // EventInfo tree
  _eventInfo = new TTree("Event", "Event information");
  _eventInfo->Branch("TimeStamp", &timeStamp, "TimeStamp/l");
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
  _tracks->Branch(
      "CovarianceX", trackCovarianceX, "TrackCovarianceX[NTracks]/D");
  _tracks->Branch(
      "CovarianceY", trackCovarianceY, "TrackCovarianceY[NTracks]/D");
  _tracks->Branch("Chi2", trackChi2, "TrackChi2[NTracks]/D");

  // SummaryTree tree
  _summaryTree = new TTree("SummaryTree", "Summary of configuration");
  _summaryTree->Branch("NRuns", &st_numRuns, "NRuns/I");
  _summaryTree->Branch("Run", st_run, "Run[NRuns]/I");

  _summaryTree->Branch("nscan_NRuns", &st_nscan_numRuns, "nscan_NRuns/I");
  _summaryTree->Branch("nscan_Run", st_nscan_run, "nscan_Run[nscan_NRuns]/I");
  _summaryTree->Branch(
      "nscan_maxFactor", &st_nscan_maxFactor, "nscan_maxFactor/D");
  _summaryTree->Branch(
      "nscan_maxOccupancy", &st_nscan_maxOccupancy, "nscan_maxOccupancy/D");
  _summaryTree->Branch("nscan_bottomX", &st_nscan_bottom_x, "nscan_bottomX/I");
  _summaryTree->Branch("nscan_upperX", &st_nscan_upper_x, "nscan_upperX/I");
  _summaryTree->Branch("nscan_bottomY", &st_nscan_bottom_y, "nscan_bottomY/I");
  _summaryTree->Branch("nscan_upperY", &st_nscan_upper_y, "nscan_upperY/I");
  _summaryTree->Branch(
      "nscan_NNoisyPixels", &st_nscan_numNoisyPixels, "nscan_NNoisyPixels/I");
  _summaryTree->Branch("nscan_noisyPixel_plane",
                       st_nscan_noisyPixel_plane.data(),
                       "nscan_noisyPixel_Plane[nscan_NNoisyPixels]/I");
  _summaryTree->Branch("nscan_noisyPixel_x",
                       st_nscan_noisyPixel_x.data(),
                       "nscan_noisyPixel_X[nscan_NNoisyPixels]/I");
  _summaryTree->Branch("nscan_noisyPixel_y",
                       st_nscan_noisyPixel_y.data(),
                       "nscan_noisyPixel_Y[nscan_NNoisyPixels]/I");
}

namespace Storage {

  //=========================================================
  StorageIO::StorageIO(const std::string& filePath,
		       Mode fileMode,
		       unsigned int numPlanes,
		       const unsigned int treeMask,
		       const std::vector<bool>* planeMask) :
    _file(NULL),
    _fileMode(fileMode),
    _numPlanes(numPlanes),
    _numEvents(0),
    _noiseMasks(0),
    _tracks(0),
    _eventInfo(0),
    _summaryTree(0)
  {

    // Plane mask holds a true for masked planes
    if(planeMask && fileMode == OUTPUT)
      throw std::runtime_error("StorageIO: can't use a plane mask in output mode");

    clearVariables();

    if (fileMode == INPUT) {
      openRead(filePath, planeMask);
    } else {
      openTruncate(filePath);
    }

    // debug info
    INFO("file path: ", _file->GetPath(), '\n');
    INFO("file mode: ", (_fileMode ? "OUTPUT" : "INPUT"), '\n');
    INFO("planes: ", _numPlanes, '\n');
    INFO("tree mask: ", treeMask, '\n');
    INFO(printSummaryTree(), '\n');

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

    assert(_hits.size() == _clusters.size() && "StorageIO: varying number of planes");

    _numEvents = 0;
    if ( _fileMode == INPUT ){ // INPUT mode
      Long64_t nEventInfo = (_eventInfo) ? _eventInfo->GetEntriesFast() : 0;
      Long64_t nTracks = (_tracks) ? _tracks->GetEntriesFast() : 0;
      Long64_t nHits = 0;
      Long64_t nClusters = 0;
      for(unsigned int nplane=0; nplane<_numPlanes; nplane++){
	if (_hits.at(nplane)) nHits += _hits.at(nplane)->GetEntriesFast();
	if (_clusters.at(nplane)) nClusters += _clusters.at(nplane)->GetEntriesFast();
      }

      if (nHits % _numPlanes || nClusters % _numPlanes)
	throw std::runtime_error("StorageIO: number of events in different planes mismatch");

      nHits /= _numPlanes;
      nClusters /= _numPlanes;

      if (!_numEvents && nEventInfo) _numEvents = nEventInfo;
      if (!_numEvents && nTracks) _numEvents = nTracks;
      if (!_numEvents && nHits) _numEvents = nHits;
      if (!_numEvents && nClusters) _numEvents = nClusters;

      if ((nEventInfo && _numEvents != nEventInfo) ||
	  (nTracks && _numEvents != nTracks) ||
	  (nHits && _numEvents != nHits) ||
	  (nClusters && _numEvents != nClusters))
	throw std::runtime_error("StorageIO: all trees don't have the same number of events");

    } // end if (_fileMode == INPUT)

  } // end of StorageIO constructor

  //=========================================================
  StorageIO::~StorageIO(){
    if (_file && _fileMode == OUTPUT){

      /* fill summaryTree here to ensure it
	 is done just once (not a per-event tree) */
      _summaryTree->Fill();

      INFO("file path: ", _file->GetPath(), '\n');
      INFO("file mode: ", (_fileMode ? "OUTPUT" : "INPUT"), '\n');
      INFO("planes: ", _numPlanes, '\n');
      INFO(printSummaryTree(), '\n');
      _file->Write();
      delete _file;
    }
  }

  //=========================================================
  void StorageIO::clearVariables(){
    timeStamp = 0;
    frameNumber = 0;
    triggerOffset = 0;
    invalid = false;

    numHits = 0;
    for(int i=0; i<MAX_HITS; i++){
      hitPixX[i]      = 0;
      hitPixY[i]      = 0;
      hitPosX[i]      = 0;
      hitPosY[i]      = 0;
      hitPosZ[i]      = 0;
      hitValue[i]     = 0;
      hitTiming[i]    = 0;
      hitInCluster[i] = 0;
    }

    numClusters = 0;
    for(int i=0; i<MAX_HITS; i++){
      clusterPixX[i]    = 0;
      clusterPixY[i]    = 0;
      clusterPixErrX[i] = 0;
      clusterPixErrY[i] = 0;
      clusterPosX[i]    = 0;
      clusterPosY[i]    = 0;
      clusterPosZ[i]    = 0;
      clusterPosErrX[i] = 0;
      clusterPosErrY[i] = 0;
      clusterPosErrZ[i] = 0;
      clusterInTrack[i] = 0;
    }

    numIntercepts = 0;
    for (int i = 0; i<MAX_HITS; ++i){
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
    for(int i=0; i<MAX_HITS; i++){
      trackSlopeX[i]      = 0;
      trackSlopeY[i]      = 0;
      trackSlopeErrX[i]   = 0;
      trackSlopeErrY[i]   = 0;
      trackOriginX[i]     = 0;
      trackOriginY[i]     = 0;
      trackOriginErrX[i]  = 0;
      trackOriginErrY[i]  = 0;
      trackCovarianceX[i] = 0;
      trackCovarianceY[i] = 0;
      trackChi2[i]        = 0;
    }

    st_numRuns       = 0;
    st_nscan_numRuns = 0;
    for(int i=0; i<MAX_RUNS; i++){
      st_run[i]       = -1;
      st_nscan_run[i] = -1;
    }
    st_nscan_maxFactor    =  0;
    st_nscan_maxOccupancy =  0;
    st_nscan_bottom_x     = -1;
    st_nscan_upper_x      = -1;
    st_nscan_bottom_y     = -1;
    st_nscan_upper_y      = -1;
    st_nscan_numNoisyPixels = 0;
    st_nscan_noisyPixel_plane.clear();
    st_nscan_noisyPixel_x.clear();
    st_nscan_noisyPixel_y.clear();
  }

  //=========================================================
  const std::string StorageIO::printSummaryTree(){
    using std::endl;
    std::ostringstream out;

    if(!_summaryTree) {
      ERROR("no summaryTree found\n");
      return out.str();
    }

    // just to need to get single entry
    _summaryTree->GetEntry(0);

    //_summaryTree->Print();

    out << "  - summaryInfo " << endl;
    out << "      * nruns = " << st_numRuns << endl;
    for(int i=0; i<st_numRuns; i++)
      out << "        o run[" << i << "] = " << st_run[i] << endl;

    out << "      * NoiseScan info" << endl;
      out << "        o runs = ";
      for(int i=0; i<st_nscan_numRuns; i++) out << st_nscan_run[i] << " ";
      out << endl;
      out << "        o maxFactor = " << st_nscan_maxFactor << endl;
      out << "        o maxOccupancy = " << st_nscan_maxOccupancy << endl;
      out << "        o bottomX = " << st_nscan_bottom_x << endl;
      out << "        o upperX = " << st_nscan_upper_x << endl;
      out << "        o bottomY = " << st_nscan_bottom_y << endl;
      out << "        o upperY = " << st_nscan_upper_y << endl;
      out << "        o nNoisyPixels = " << st_nscan_numNoisyPixels << endl;
      for(int i=0; i<st_nscan_numNoisyPixels; i++){
	out << "          -> pixel["
	     << st_nscan_noisyPixel_plane[i] << ","
	     << st_nscan_noisyPixel_x[i] << ","
	     << st_nscan_noisyPixel_y[i] << "]" << endl;
    }
      out << endl;
      return out.str();
  }

  //=========================================================
  void StorageIO::setRuns(const std::vector<int> &vruns){
    assert( _fileMode != INPUT && "StorageIO: can't set runs in input mode" );

    if( vruns.empty() ){
      ERROR("no --runs option detected; summaryTree saved without run info\n");
      return;
    }

    st_numRuns=0;
    std::vector<int>::const_iterator cit;
    for(cit=vruns.begin(); cit!=vruns.end(); ++cit){
      st_run[st_numRuns] = (*cit);
      st_numRuns++;
    }
  }

  //=========================================================
  void StorageIO::setNoiseMaskData(const Mechanics::NoiseMask& noisemask)
  {
    const Loopers::NoiseScanConfig* config = NULL;
    if (!config) {
      ERROR("[StorageIO::setNoiseMaskData] WARNING: no NoiseScanConfig info found\n");
      return;
    }

    st_nscan_numRuns = 0;
    const std::vector<int> runs = config->getRuns();
    for (std::vector<int>::const_iterator cit = runs.begin();
         cit != runs.end(); ++cit, st_nscan_numRuns++)
      st_nscan_run[st_nscan_numRuns] = *cit;

    st_nscan_maxFactor = config->getMaxFactor();
    st_nscan_maxOccupancy = config->getMaxOccupancy();
    st_nscan_bottom_x = config->getBottomLimitX();
    st_nscan_upper_x = config->getUpperLimitX();
    st_nscan_bottom_y = config->getBottomLimitY();
    st_nscan_upper_y = config->getUpperLimitY();

    st_nscan_numNoisyPixels = 0;
    st_nscan_noisyPixel_plane.clear();
    st_nscan_noisyPixel_x.clear();
    st_nscan_noisyPixel_y.clear();
    for (unsigned int isensor = 0; isensor < _numPlanes; ++isensor) {
      const auto& maskedIndices = noisemask.getMaskedPixels(isensor);
      for (const auto& index : maskedIndices) {
        st_nscan_noisyPixel_plane.push_back(isensor);
        st_nscan_noisyPixel_x.push_back(std::get<0>(index));
        st_nscan_noisyPixel_y.push_back(std::get<1>(index));
        st_nscan_numNoisyPixels += 1;
      }
    }
    // changing the sizes of the vectors can lead to reallocated memory
    // ensure the correct values are stored
    _summaryTree->SetBranchAddress("nscan_noisyPixel_plane", st_nscan_noisyPixel_plane.data(), &b_NoiseScan_NoisyPixelPlane);
    _summaryTree->SetBranchAddress("nscan_noisyPixel_x", st_nscan_noisyPixel_x.data(), &b_NoiseScan_NoisyPixelX);
    _summaryTree->SetBranchAddress("nscan_noisyPixel_y", st_nscan_noisyPixel_y.data(), &b_NoiseScan_NoisyPixelY);
  }

  //=========================================================
  std::vector<int> StorageIO::getRuns() const {
    std::vector<int> vec;
    if(!_summaryTree){
      ERROR("[StorageIO::getRuns()] WARNING no summaryTree, can't get run info\n");
    }
    else{
      _summaryTree->GetEntry(0);
      for(int i=0; i<st_numRuns; i++)
	vec.push_back(st_run[i]);
    }
    return vec;
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
    event->setTimeStamp(timeStamp);
    event->setFrameNumber(frameNumber);
    event->setTriggerOffset(triggerOffset);
    event->setTriggerInfo(triggerInfo);
    event->setTriggerPhase(triggerPhase);
    event->setInvalid(invalid);

    // Generate a list of track objects
    for (int ntrack = 0; ntrack < numTracks; ntrack++) {
      TrackState state(trackOriginX[ntrack],
                       trackOriginY[ntrack],
                       trackSlopeX[ntrack],
                       trackSlopeY[ntrack]);
      state.setErrU(trackOriginErrX[ntrack],
                    trackSlopeErrX[ntrack],
                    trackCovarianceX[ntrack]);
      state.setErrV(trackOriginErrY[ntrack],
                    trackSlopeErrY[ntrack],
                    trackCovarianceY[ntrack]);
      Track* track = event->newTrack();
      track->setGlobalState(state);
      track->setGoodnessOfFit(trackChi2[ntrack]);
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
        Storage::TrackState local(interceptU[iintercept],
                                  interceptV[iintercept],
                                  interceptSlopeU[iintercept],
                                  interceptSlopeV[iintercept]);
        local.setErrU(
            interceptStdU[iintercept], interceptStdSlopeU[iintercept], 0);
        local.setErrV(
            interceptStdV[iintercept], interceptStdSlopeV[iintercept], 0);
        event->getTrack(interceptTrack[iintercept])
            ->addLocalState(nplane, std::move(local));
      }

      // Generate the cluster objects
      for (int ncluster = 0; ncluster < numClusters; ncluster++) {
        Cluster* cluster = plane->newCluster();
        cluster->setPosPixel(clusterPixX[ncluster], clusterPixY[ncluster]);
        cluster->setErrPixel(
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
        if (_noiseMasks &&
            _noiseMasks->at(nplane)[hitPixX[nhit]][hitPixY[nhit]]) {
          if (hitInCluster[nhit] >= 0)
            throw std::runtime_error(
                "StorageIO: tried to mask a hit which is already in a cluster");
          continue;
        }

        Hit* hit = plane->newHit();
        hit->setAddress(hitPixX[nhit], hitPixY[nhit]);
        // hit->setPosGlobal({hitPosX[nhit], hitPosY[nhit], hitPosZ[nhit]});
        hit->setValue(hitValue[nhit]);
        hit->setTiming(hitTiming[nhit]);

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

    timeStamp = event->getTimeStamp();
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
      numIntercepts = 0;
      for (Index itrack = 0; itrack < event->numTracks(); ++itrack) {
        const Track* track = event->getTrack(itrack);
        if (!track->hasLocalState(nplane))
          continue;
        const TrackState& local = track->localState(nplane);
        interceptTrack[numIntercepts] = itrack;
        interceptU[numIntercepts] = local.offset().x();
        interceptV[numIntercepts] = local.offset().y();
        interceptSlopeU[numIntercepts] = local.slope().x();
        interceptSlopeV[numIntercepts] = local.slope().y();
        interceptStdU[numIntercepts] = local.stdOffsetU();
        interceptStdV[numIntercepts] = local.stdOffsetV();
        interceptStdSlopeU[numIntercepts] = local.stdSlopeU();
        interceptStdSlopeV[numIntercepts] = local.stdSlopeV();
        numIntercepts += 1;
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
        clusterPosX[ncluster] = cluster->getPosX();
        clusterPosY[ncluster] = cluster->getPosY();
        clusterPosZ[ncluster] = cluster->getPosZ();
        clusterPosErrX[ncluster] = cluster->getPosErrX();
        clusterPosErrY[ncluster] = cluster->getPosErrY();
        clusterPosErrZ[ncluster] = cluster->getPosErrZ();
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
        hitPosX[nhit] = hit->getPosX();
        hitPosY[nhit] = hit->getPosY();
        hitPosZ[nhit] = hit->getPosZ();
        hitValue[nhit] = hit->value();
        hitTiming[nhit] = hit->timing();
        hitInCluster[nhit] =
            hit->isInCluster() ? hit->cluster()->getIndex() : -1;
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
