#include "storageio.h"
#include "../configparser.h"

#include <cassert>
#include <sstream>
#include <vector>
#include <iostream>
#include <stdlib.h>

#include <TFile.h>
#include <TDirectory.h>
#include <TTree.h>
#include <TBranch.h>

#include "event.h"
#include "track.h"
#include "plane.h"
#include "cluster.h"
#include "hit.h"
#include "../mechanics/noisemask.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../loopers/noisescan.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

namespace Storage {

  //=========================================================
  StorageIO::StorageIO(const char* filePath,
		       Mode fileMode,
		       unsigned int numPlanes,
		       const unsigned int treeMask,
		       const std::vector<bool>* planeMask,
		       int printLevel) :
    _filePath(filePath),
    _file(0),
    _fileMode(fileMode),
    _numPlanes(0),
    _numEvents(0),
    _printLevel(printLevel),
    _noiseMasks(0),
    _tracks(0),
    _eventInfo(0),
    _summaryTree(0)
  {
    if      (fileMode == INPUT)  _file = new TFile(_filePath, "READ");
    else if (fileMode == OUTPUT) _file = new TFile(_filePath, "RECREATE");
    if (!_file) throw "StorageIO: file didn't initialize";

    // clear tree variables
    clearVariables();

    // Plane mask holds a true for masked planes
    if(planeMask && fileMode == OUTPUT)
      throw "StorageIO: can't use a plane mask in output mode";

    /****************************************************
     In OUTPUT mode,
     create the directory structure and the relevant trees
    ****************************************************/
    if ( _fileMode == OUTPUT ){
      if (planeMask && (VERBOSE || _printLevel>0) )
	cout << "WARNING :: StorageIO: disregarding plane mask in output mode";

      _numPlanes = numPlanes;

      TDirectory* dir = 0;
      for(unsigned int nplane=0; nplane<_numPlanes; nplane++){
	std::stringstream ss;
	ss << "Plane" << nplane;

	dir = _file->mkdir(ss.str().c_str());
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

	// Local track position tree
	TTree* intercepts = new TTree("Intercepts", "Intercepts");
	_intercepts.push_back(intercepts);
	intercepts->Branch("NIntercepts", &numIntercepts, "NIntercepts/I");
	intercepts->Branch("interceptX", interceptX, "interceptX[NIntercepts]/D");
	intercepts->Branch("interceptY", interceptY, "interceptY[NIntercepts]/D");
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
      _tracks->Branch("CovarianceX", trackCovarianceX, "TrackCovarianceX[NTracks]/D");
      _tracks->Branch("CovarianceY", trackCovarianceY, "TrackCovarianceY[NTracks]/D");
      _tracks->Branch("Chi2", trackChi2, "TrackChi2[NTracks]/D");

      // SummaryTree tree
      _summaryTree = new TTree("SummaryTree", "Summary of configuration");
      _summaryTree->Branch("NRuns", &st_numRuns, "NRuns/I");
      _summaryTree->Branch("Run", st_run, "Run[NRuns]/I");

      _summaryTree->Branch("nscan_NRuns", &st_nscan_numRuns, "nscan_NRuns/I");
      _summaryTree->Branch("nscan_Run", st_nscan_run, "nscan_Run[nscan_NRuns]/I");
      _summaryTree->Branch("nscan_maxFactor", &st_nscan_maxFactor, "nscan_maxFactor/D");
      _summaryTree->Branch("nscan_maxOccupancy", &st_nscan_maxOccupancy, "nscan_maxOccupancy/D");
      _summaryTree->Branch("nscan_bottomX", &st_nscan_bottom_x, "nscan_bottomX/I");
      _summaryTree->Branch("nscan_upperX", &st_nscan_upper_x, "nscan_upperX/I");
      _summaryTree->Branch("nscan_bottomY", &st_nscan_bottom_y, "nscan_bottomY/I");
      _summaryTree->Branch("nscan_upperY", &st_nscan_upper_y, "nscan_upperY/I");
      _summaryTree->Branch("nscan_NNoisyPixels",  &st_nscan_numNoisyPixels, "nscan_NNoisyPixels/I");
      _summaryTree->Branch("nscan_noisyPixel_plane", st_nscan_noisyPixel_plane.data(), "nscan_noisyPixel_Plane[nscan_NNoisyPixels]/I");
      _summaryTree->Branch("nscan_noisyPixel_x", st_nscan_noisyPixel_x.data(), "nscan_noisyPixel_X[nscan_NNoisyPixels]/I");
      _summaryTree->Branch("nscan_noisyPixel_y", st_nscan_noisyPixel_y.data(), "nscan_noisyPixel_Y[nscan_NNoisyPixels]/I");

    } // end if (_fileMode == OUTPUT)

    /*********************
     In input mode
    *********************/
    if (_fileMode == INPUT){

      if (_numPlanes && (VERBOSE || _printLevel>0) )
	cout << "WARNING :: StorageIO: disregarding specified number of planes" << endl;

      _numPlanes = 0; // Determine num planes from file structure

      unsigned int planeCount = 0;
      while (true){
	std::stringstream ss;
	ss << "Plane" << planeCount;

	// Try to get this plane's directory
	TDirectory* dir = 0;
	_file->GetObject(ss.str().c_str(), dir);
	if (!dir) break;

	planeCount++;

	if (planeMask && planeCount > planeMask->size())
	  throw "StorageIO: plane mask is too small";

	if (planeMask && planeMask->at(planeCount - 1)) continue;

	_numPlanes++;

	TTree* hits;
	_file->GetObject(ss.str().append("/Hits").c_str(), hits);
	_hits.push_back(hits);
	if (hits){
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
	_file->GetObject(ss.str().append("/Clusters").c_str(), clusters);
	_clusters.push_back(clusters);
	if(clusters){
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
	_file->GetObject(ss.str().append("/Intercepts").c_str(), intercepts);
	_intercepts.push_back(intercepts);
	if(intercepts){
	  intercepts->SetBranchAddress("NIntercepts", &numIntercepts, &bNumIntercepts);
	  intercepts->SetBranchAddress("interceptX", interceptX, &bInterceptX);
	  intercepts->SetBranchAddress("interceptY", interceptY, &bInterceptY);
	}


}

      _file->GetObject("Event", _eventInfo);
      if (_eventInfo){
	_eventInfo->SetBranchAddress("TimeStamp", &timeStamp, &bTimeStamp);
	_eventInfo->SetBranchAddress("FrameNumber", &frameNumber, &bFrameNumber);
	_eventInfo->SetBranchAddress("TriggerOffset", &triggerOffset, &bTriggerOffset);
	_eventInfo->SetBranchAddress("TriggerInfo", &triggerInfo, &bTriggerInfo);
	_eventInfo->SetBranchAddress("TriggerPhase", &triggerPhase, &bTriggerPhase);
	_eventInfo->SetBranchAddress("Invalid", &invalid, &bInvalid);
      }

      _file->GetObject("Tracks", _tracks);
      if (_tracks){
	_tracks->SetBranchAddress("NTracks", &numTracks, &bNumTracks);
	_tracks->SetBranchAddress("SlopeX", trackSlopeX, &bTrackSlopeX);
	_tracks->SetBranchAddress("SlopeY", trackSlopeY, &bTrackSlopeY);
	_tracks->SetBranchAddress("SlopeErrX", trackSlopeErrX, &bTrackSlopeErrX);
	_tracks->SetBranchAddress("SlopeErrY", trackSlopeErrY, &bTrackSlopeErrY);
	_tracks->SetBranchAddress("OriginX", trackOriginX, &bTrackOriginX);
	_tracks->SetBranchAddress("OriginY", trackOriginY, &bTrackOriginY);
	_tracks->SetBranchAddress("OriginErrX", trackOriginErrX, &bTrackOriginErrX);
	_tracks->SetBranchAddress("OriginErrY", trackOriginErrY, &bTrackOriginErrY);
	_tracks->SetBranchAddress("CovarianceX", trackCovarianceX, &bTrackCovarianceX);
	_tracks->SetBranchAddress("CovarianceY", trackCovarianceY, &bTrackCovarianceY);
	_tracks->SetBranchAddress("Chi2", trackChi2, &bTrackChi2);
      }

      _file->GetObject("SummaryTree", _summaryTree);
      if( _summaryTree ){
	_summaryTree->SetBranchAddress("NRuns", &st_numRuns, &b_NumRuns);
	_summaryTree->SetBranchAddress("Run", st_run, &b_Run);
	_summaryTree->SetBranchAddress("nscan_NRuns", &st_nscan_numRuns, &b_NoiseScan_NumRuns);
	_summaryTree->SetBranchAddress("nscan_Run", st_nscan_run, &b_NoiseScan_Run);
	_summaryTree->SetBranchAddress("nscan_maxFactor", &st_nscan_maxFactor, &b_NoiseScan_MaxFactor);
	_summaryTree->SetBranchAddress("nscan_maxOccupancy", &st_nscan_maxOccupancy, &b_NoiseScan_MaxOccupancy);
	_summaryTree->SetBranchAddress("nscan_bottomX", &st_nscan_bottom_x, &b_NoiseScan_BottomX);
	_summaryTree->SetBranchAddress("nscan_upperX", &st_nscan_upper_x, &b_NoiseScan_UpperX);
	_summaryTree->SetBranchAddress("nscan_bottomY", &st_nscan_bottom_y, &b_NoiseScan_BottomY);
	_summaryTree->SetBranchAddress("nscan_upperY", &st_nscan_upper_y, &b_NoiseScan_UpperY);
	_summaryTree->SetBranchAddress("nscan_NNoisyPixels", &st_nscan_numNoisyPixels, &b_NoiseScan_NumNoisyPixels);
  // ensure enough space is available for the maximum number of possible
  // noise pixels, i.e. #planes * #pixels_per_plane
  // use Mimosa26 (1152 cols x 576 rows) as upper limit
  size_t max_size = _numPlanes * 1152 * 576;
  st_nscan_noisyPixel_plane.resize(max_size);
  st_nscan_noisyPixel_x.resize(max_size);
  st_nscan_noisyPixel_y.resize(max_size);
	_summaryTree->SetBranchAddress("nscan_noisyPixel_plane", st_nscan_noisyPixel_plane.data(), &b_NoiseScan_NoisyPixelPlane);
	_summaryTree->SetBranchAddress("nscan_noisyPixel_x", st_nscan_noisyPixel_x.data(), &b_NoiseScan_NoisyPixelX);
	_summaryTree->SetBranchAddress("nscan_noisyPixel_y", st_nscan_noisyPixel_y.data(), &b_NoiseScan_NoisyPixelY);
      }
    } // end if(_fileMode == INPUT)

    // debug info
    if( _printLevel > 0  ){
      cout << "\n[StorageIO::StorageIO]" << endl;
      cout << "  - filePath  = " << _filePath  << endl;
      cout << "  - fileMode  = " << _fileMode;
      std::string ost = _fileMode ? "OUTPUT" : "INPUT";
      cout << " (" << ost << ")"<< endl;
      cout << "  - numPlanes = " << _numPlanes << endl;
      cout << "  - treeMask  = " << treeMask  << endl;
      cout << printSummaryTree() << endl;
    }

    if (_numPlanes < 1)
      throw "StorageIO: didn't initialize any planes";

    // Delete trees as per the tree flags
    if( treeMask ){
      for(unsigned int nplane=0; nplane<_numPlanes; nplane++){
	if (treeMask & Flags::HITS) { delete _hits.at(nplane); _hits.at(nplane) = 0; }
	if (treeMask & Flags::CLUSTERS) { delete _clusters.at(nplane); _clusters.at(nplane) = 0; }
      }
      if (treeMask & Flags::TRACKS) { delete _tracks; _tracks = 0; }
      if (treeMask & Flags::EVENTINFO) { delete _eventInfo; _eventInfo = 0; }
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
	throw "StorageIO: number of events in different planes mismatch";

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
	throw "StorageIO: all trees don't have the same number of events";

    } // end if (_fileMode == INPUT)

  } // end of StorageIO constructor

  //=========================================================
  StorageIO::~StorageIO(){
    if (_file && _fileMode == OUTPUT){

      /* fill summaryTree here to ensure it
	 is done just once (not a per-event tree) */
      _summaryTree->Fill();

      if( _printLevel>1 ){
	cout << "\n[Storage::~Storage]"<< endl;
	cout << "  - filePath  = " << _filePath  << endl;
	cout << "  - fileMode  = " << _fileMode;
	std::string ost = _fileMode ? "OUTPUT" : "INPUT";
	cout << " (" << ost << ")"<< endl;
	cout << "  - numPlanes = " << _numPlanes << endl;
	cout << printSummaryTree() << endl;
      }
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
    for (int i = 0; i<MAX_HITS; i++){
		interceptX[i] = 0;
		interceptY[i] = 0;

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
    std::ostringstream out;

    if( _summaryTree == 0 ){
      out << "[StorageIO::printSummaryTree] WARNING :: "
	  << "no summaryTree found  " << endl;
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
  //
  // //=========================================================
  // void StorageIO::setNoiseMasks(std::vector<bool**>* noiseMasks){
  //   if (noiseMasks && _numPlanes != noiseMasks->size())
  //     throw "StorageIO: noise mask has more planes than will be read in";
  //   _noiseMasks = noiseMasks;
  // }

  //=========================================================
  void StorageIO::setPrintLevel(const int printLevel){
    _printLevel = printLevel;
  }

  //=========================================================
  void StorageIO::setRuns(const std::vector<int> &vruns){
    assert( _fileMode != INPUT && "StorageIO: can't set runs in input mode" );

    if( vruns.empty() ){
      cout << "WARNING :: no --runs option detected; "
	   << "summaryTree saved without run info" << endl;
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
      cout << "[StorageIO::setNoiseMaskData] WARNING: no NoiseScanConfig "
              "info found"
           << endl;
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
  Long64_t StorageIO::getNumEvents() const {
    assert(_fileMode != OUTPUT && "StorageIO: can't get number of entries in output mode");
    return _numEvents;
  }

  //=========================================================
  std::vector<int> StorageIO::getRuns() const {
    std::vector<int> vec;
    if(!_summaryTree){
      cout << "[StorageIO::getRuns()] WARNING no summaryTree, can't get run info" << endl;
    }
    else{
      _summaryTree->GetEntry(0);
      for(int i=0; i<st_numRuns; i++)
	vec.push_back(st_run[i]);
    }
    return vec;
  }

  //=========================================================
  Event* StorageIO::readEvent(Long64_t n)
  {
    /* Note: fill in reversed order: tracks first, hits last. This is so that
     * once a hit is produced, it can immediately recieve the address of its
     * parent cluster, likewise for clusters and track. */

    if (n >= _numEvents) throw "StorageIO: requested event outside range";

    if (_eventInfo &&_eventInfo->GetEntry(n) <= 0) throw "StorageIO: error reading event tree";
    if (_tracks && _tracks->GetEntry(n) <= 0) throw "StorageIO: error reading tracks tree";

    Event* event = new Event(_numPlanes);
    event->setTimeStamp(timeStamp);
    event->setFrameNumber(frameNumber);
    event->setTriggerOffset(triggerOffset);
    event->setTriggerInfo(triggerInfo);
    event->setTriggerPhase(triggerPhase);

    event->setInvalid(invalid);

    // Generate a list of track objects
    for (int ntrack=0; ntrack<numTracks; ntrack++){
      Track* track = event->newTrack();
      track->setOrigin(trackOriginX[ntrack], trackOriginY[ntrack]);
      track->setOriginErr(trackOriginErrX[ntrack], trackOriginErrY[ntrack]);
      track->setSlope(trackSlopeX[ntrack], trackSlopeY[ntrack]);
      track->setSlopeErr(trackSlopeErrX[ntrack], trackSlopeErrY[ntrack]);
      track->setCovariance(trackCovarianceX[ntrack], trackCovarianceY[ntrack]);
      track->setChi2(trackChi2[ntrack]);
    }

    for (unsigned int nplane=0; nplane<_numPlanes; nplane++){

      if (_hits.at(nplane) && _hits.at(nplane)->GetEntry(n) <= 0)
	throw "StorageIO: error reading hits tree";

      if (_clusters.at(nplane) && _clusters.at(nplane)->GetEntry(n) <= 0)
	throw "StorageIO: error reading clusters tree";

      if (_intercepts.at(nplane)){
         if(_intercepts.at(nplane)->GetEntry(n) <= 0){
            throw "StorageIO: error reading intercepts tree";
         }
         // Add intercepts
         for(int nintercept=0; nintercept < numIntercepts; nintercept++) {
            event->getPlane(nplane)->addIntercept(interceptX[nintercept],
                   interceptY[nintercept]);
         }
      }

   // Generate the cluster objects
   for (int ncluster=0; ncluster<numClusters; ncluster++){
	   Cluster* cluster = event->newCluster(nplane);
      cluster->setPix(clusterPixX[ncluster], clusterPixY[ncluster]);
	   cluster->setPixErr(clusterPixErrX[ncluster], clusterPixErrY[ncluster]);
	   cluster->setPos(clusterPosX[ncluster], clusterPosY[ncluster], clusterPosZ[ncluster]);
	   cluster->setPosErr(clusterPosErrX[ncluster], clusterPosErrY[ncluster], clusterPosErrZ[ncluster]);

	// If this cluster is in a track, mark this (and the tracks tree is active)
	if (_tracks && clusterInTrack[ncluster] >= 0){
	  Track* track = event->getTrack(clusterInTrack[ncluster]);
	  track->addCluster(cluster);
	  cluster->setTrack(track);
	}
   }

      // Generate a list of all hit objects
      for(int nhit=0; nhit<numHits; nhit++) {
	if (_noiseMasks && _noiseMasks->at(nplane)[hitPixX[nhit]][hitPixY[nhit]]){
	  if (hitInCluster[nhit] >= 0)
	    throw "StorageIO: tried to mask a hit which is already in a cluster";
	  continue;
	}

	Hit* hit = event->newHit(nplane);
	hit->setPix(hitPixX[nhit], hitPixY[nhit]);
	hit->setPos(hitPosX[nhit], hitPosY[nhit], hitPosZ[nhit]);
	hit->setValue(hitValue[nhit]);
	hit->setTiming(hitTiming[nhit]);

	// If this hit is in a cluster, mark this (and the clusters tree is active)
	if (_clusters.at(nplane) && hitInCluster[nhit] >= 0){
	  Cluster* cluster = event->getCluster(hitInCluster[nhit]);
	  cluster->addHit(hit);
	}
      }
    } // end loop in planes

    return event;
  }

  //=========================================================
  void StorageIO::writeEvent(Event* event){

    if (_fileMode == INPUT) throw "StorageIO: can't write event in input mode";

    timeStamp = event->getTimeStamp();
    frameNumber = event->getFrameNumber();
    triggerOffset = event->getTriggerOffset();
    triggerInfo = event->getTriggerInfo();
    triggerPhase = event->getTriggerPhase();
    //cout<<triggerPhase<<endl;
    invalid = event->getInvalid();

    numTracks = event->getNumTracks();
    if (numTracks > MAX_TRACKS) throw "StorageIO: event exceeds MAX_TRACKS";

    // Set the object track values into the arrays for writing to the root file
    for (int ntrack=0; ntrack<numTracks; ntrack++){
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

    for (unsigned int nplane=0; nplane<_numPlanes; nplane++){
      Plane* plane = event->getPlane(nplane);

      // fill intercepts
		numIntercepts = plane->getNumIntercepts();
		if (numIntercepts > MAX_TRACKS) throw "StorageIO: event exceeds MAX_Tracks";
		for (int nintercept = 0; nintercept < numIntercepts; nintercept++) {
			std::pair<double, double> intercept = plane->getIntercept(nintercept);
			interceptX[nintercept] = intercept.first;
			interceptY[nintercept] = intercept.second;
		}

      numClusters = plane->getNumClusters();
      if (numClusters > MAX_CLUSTERS) throw "StorageIO: event exceeds MAX_CLUSTERS";

      // Set the object cluster values into the arrays for writig into the root file
      for (int ncluster=0; ncluster<numClusters; ncluster++)
	{
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
	  clusterInTrack[ncluster] = cluster->getTrack() ? cluster->getTrack()->getIndex() : -1;
	}

      numHits = plane->getNumHits();
      if (numHits > MAX_HITS) throw "StorageIO: event exceeds MAX_HITS";

      // Set the object hit values into the arrays for writing into the root file
      for (int nhit=0; nhit<numHits; nhit++){
	Hit* hit = plane->getHit(nhit);
	hitPixX[nhit] = hit->getPixX();
	hitPixY[nhit] = hit->getPixY();
	hitPosX[nhit] = hit->getPosX();
	hitPosY[nhit] = hit->getPosY();
	hitPosZ[nhit] = hit->getPosZ();
	hitValue[nhit] = hit->getValue();
	hitTiming[nhit] = hit->getTiming();
	hitInCluster[nhit] = hit->getCluster() ? hit->getCluster()->getIndex() : -1;
      }

      if (nplane >= _hits.size()) throw "StorageIO: event has too many planes for the storage";

      // Fill the plane by plane trees for this plane
      if (_hits.at(nplane)) _hits.at(nplane)->Fill();
      if (_clusters.at(nplane)) _clusters.at(nplane)->Fill();
      if (_intercepts.at(nplane)) _intercepts.at(nplane)->Fill();


    }//end of nplane loop

    // Write the track and event info here so that if any errors occured they won't be desynchronized
    if (_tracks) _tracks->Fill();
    if (_eventInfo) _eventInfo->Fill();

    _numEvents++;

  }// end of writeEvent

} // end of namespace
