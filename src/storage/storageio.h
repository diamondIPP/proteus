#ifndef STORAGEIO_H
#define STORAGEIO_H

#include <vector>

#include <Rtypes.h>
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>

/* NOTE: these sizes are used to initialize arrays of track, cluster and
 * hit information. BUT these arrays are generated ONLY ONCE and re-used
 * to load events. Vectors could have been used in the ROOT file format, but
 * they would need to be constructed at each event reading step. */
#define MAX_TRACKS 1000
#define MAX_CLUSTERS 1000
#define MAX_HITS 1000

#define MAX_RUNS 1000
#define MAX_NOISY 10000

class ConfigParser;
namespace Mechanics { class NoiseMask; }

namespace Storage {

  class Event;
  
  enum Mode {
    INPUT=0,
    OUTPUT
  };
  
  namespace Flags {
    enum TreeFlags{
      NONE      = 0x0,
      HITS      = 0x1,
      CLUSTERS  = 0x2,
      TRACKS    = 0x4,
      EVENTINFO = 0x8
    };
  }
  
  class StorageIO {
  public:
    /** Constructor. */
    StorageIO(const char* filePath,
	      Mode fileMode,
	      unsigned int numPlanes = 0,
	      const unsigned int treeMask = 0,
	      const std::vector<bool>* planeMask = 0,
	      int printLevel = 0);

    /** Destructor */
    ~StorageIO();
    
    void setNoiseMasks(std::vector<bool**>* noiseMasks);
    void setPrintLevel(const int printLevel);
    void setRuns(const std::vector<int> &vruns);
    void setNoiseMaskData(const Mechanics::NoiseMask* noisemask);
    
    Long64_t getNumEvents() const;
    unsigned int getNumPlanes() const { return _numPlanes; }
    Storage::Mode getMode() const {  return _fileMode; }
    std::vector<int> getRuns() const;

    Event* readEvent(Long64_t n); // Read an event and generate its objects
    void writeEvent(Event* event); // Write an event at the end of the file

  private:
    StorageIO(const StorageIO&); // Disable the copy constructor
    StorageIO& operator=(const StorageIO&); // Disable the assignment operator
    void clearVariables();
    const std::string printSummaryTree();

  private:
    const char*  _filePath; // Path to the storage file
    TFile*       _file; // Storage file
    const Mode   _fileMode; // How to open and process the file
    unsigned int _numPlanes; // This can be read from the file structure
    Long64_t     _numEvents; // Number of events in the input file

    int _printLevel; // verbose level

    const std::vector<bool**>* _noiseMasks;

    /* NOTE: trees can easily be added and removed from a file. So each type
     * of information that might or might not be included in a file should be
     * in its own tree. */
    
    // Trees containing event-by-event data for each plane
    std::vector<TTree*> _hits;
    std::vector<TTree*> _clusters;
    std::vector<TTree*> _intercepts;
    // Trees global to the entire event
    TTree*  _tracks;
    TTree*  _eventInfo;

    // Tree with configuration info
    TTree* _summaryTree;

    //
    // Variables in which the storage is output on an event-by-event basis
    //
    // HITS
    Int_t    numHits;
    Int_t    hitPixX[MAX_HITS];
    Int_t    hitPixY[MAX_HITS];
    Double_t hitPosX[MAX_HITS];
    Double_t hitPosY[MAX_HITS];
    Double_t hitPosZ[MAX_HITS];
    Int_t    hitValue[MAX_HITS];
    Int_t    hitTiming[MAX_HITS];
    Int_t    hitInCluster[MAX_HITS];

    // CLUSTERS
    Int_t    numClusters;
    Double_t clusterPixX[MAX_CLUSTERS];
    Double_t clusterPixY[MAX_CLUSTERS];
    Double_t clusterPixErrX[MAX_CLUSTERS];
    Double_t clusterPixErrY[MAX_CLUSTERS];
    Double_t clusterPosX[MAX_CLUSTERS];
    Double_t clusterPosY[MAX_CLUSTERS];
    Double_t clusterPosZ[MAX_CLUSTERS];
    Double_t clusterPosErrX[MAX_CLUSTERS];
    Double_t clusterPosErrY[MAX_CLUSTERS];
    Double_t clusterPosErrZ[MAX_CLUSTERS];
    Int_t    clusterInTrack[MAX_CLUSTERS];
    
    // INTERCEPTS (LOCAL HIT POSITIONS)
	Int_t numIntercepts;
	Double_t interceptX[MAX_TRACKS];
	Double_t interceptY[MAX_TRACKS];

    // EVENT INFO
    ULong64_t timeStamp;
    ULong64_t frameNumber;
    Int_t     triggerOffset;
    Int_t     triggerInfo;
    Int_t     triggerPhase;
    Bool_t    invalid;

    // TRACKS
    Int_t    numTracks;
    Double_t trackSlopeX[MAX_TRACKS];
    Double_t trackSlopeY[MAX_TRACKS];
    Double_t trackSlopeErrX[MAX_TRACKS];
    Double_t trackSlopeErrY[MAX_TRACKS];
    Double_t trackOriginX[MAX_TRACKS];
    Double_t trackOriginY[MAX_TRACKS];
    Double_t trackOriginErrX[MAX_TRACKS];
    Double_t trackOriginErrY[MAX_TRACKS];
    Double_t trackCovarianceX[MAX_TRACKS];
    Double_t trackCovarianceY[MAX_TRACKS];
    Double_t trackChi2[MAX_TRACKS];

    // Variables for summaryTree (output just once)
    Int_t st_numRuns;
    Int_t st_run[MAX_RUNS];

    Int_t st_nscan_numRuns;
    Int_t st_nscan_run[MAX_RUNS];
    Double_t st_nscan_maxFactor;
    Double_t st_nscan_maxOccupancy;
    Int_t st_nscan_bottom_x;
    Int_t st_nscan_upper_x;
    Int_t st_nscan_bottom_y;
    Int_t st_nscan_upper_y;
    Int_t st_nscan_numNoisyPixels;              // total number of noisy pixels 
    Int_t st_nscan_noisyPixel_plane[MAX_NOISY]; // noisy pixel plane number
    Int_t st_nscan_noisyPixel_x[MAX_NOISY];     // noisy pixel x (col number) 
    Int_t st_nscan_noisyPixel_y[MAX_NOISY];     // noisy pixel y (row number)

    // Branches corresponding to the above variables    
    TBranch* bNumHits;
    TBranch* bHitPixX;
    TBranch* bHitPixY;
    TBranch* bHitPosX;
    TBranch* bHitPosY;
    TBranch* bHitPosZ;
    TBranch* bHitValue;
    TBranch* bHitTiming;
    TBranch* bHitInCluster;
    
    TBranch* bNumClusters;
    TBranch* bClusterPixX;
    TBranch* bClusterPixY;
    TBranch* bClusterPixErrX;
    TBranch* bClusterPixErrY;
    TBranch* bClusterPosX;
    TBranch* bClusterPosY;
    TBranch* bClusterPosZ;
    TBranch* bClusterPosErrX;
    TBranch* bClusterPosErrY;
    TBranch* bClusterPosErrZ;
    TBranch* bClusterInTrack;

	TBranch* bNumIntercepts;
    TBranch* bInterceptX;
    TBranch* bInterceptY; 
    
    
    TBranch* bTimeStamp;
    TBranch* bFrameNumber;
    TBranch* bTriggerOffset;
    TBranch* bTriggerInfo;
    TBranch* bTriggerPhase;
    TBranch* bInvalid;
    
    TBranch* bNumTracks;
    TBranch* bTrackSlopeX;
    TBranch* bTrackSlopeY;
    TBranch* bTrackSlopeErrX;
    TBranch* bTrackSlopeErrY;
    TBranch* bTrackOriginX;
    TBranch* bTrackOriginY;
    TBranch* bTrackOriginErrX;
    TBranch* bTrackOriginErrY;
    TBranch* bTrackCovarianceX;
    TBranch* bTrackCovarianceY;
    TBranch* bTrackChi2;

    TBranch* b_NumRuns;
    TBranch* b_Run;
    TBranch* b_NoiseScan_NumRuns;
    TBranch* b_NoiseScan_Run;
    TBranch* b_NoiseScan_MaxFactor;
    TBranch* b_NoiseScan_MaxOccupancy;
    TBranch* b_NoiseScan_BottomX;
    TBranch* b_NoiseScan_UpperX;
    TBranch* b_NoiseScan_BottomY;
    TBranch* b_NoiseScan_UpperY;
    TBranch* b_NoiseScan_NumNoisyPixels;
    TBranch* b_NoiseScan_NoisyPixelPlane;
    TBranch* b_NoiseScan_NoisyPixelX;
    TBranch* b_NoiseScan_NoisyPixelY;

  };

} // end of namespace

#endif // STORAGEIO_H
