#ifndef PT_STORAGEIO_H
#define PT_STORAGEIO_H

#include <cstdint>
#include <string>
#include <vector>

#include <TBranch.h>
#include <TFile.h>
#include <TTree.h>

/* NOTE: these sizes are used to initialize arrays of track, cluster and
 * hit information. BUT these arrays are generated ONLY ONCE and re-used
 * to load events. Vectors could have been used in the ROOT file format, but
 * they would need to be constructed at each event reading step. */
#define MAX_TRACKS 10000
#define MAX_CLUSTERS 10000
#define MAX_HITS 10000

#define MAX_RUNS 1000
#define MAX_NOISY 20000

namespace Storage {

class Event;

enum Mode { INPUT = 0, OUTPUT };

namespace Flags {
enum TreeFlags {
  NONE = 0x0,
  HITS = 0x1,
  CLUSTERS = 0x2,
  TRACKS = 0x4,
  EVENTINFO = 0x8
};
}

class StorageIO {
public:
  /** Constructor. */
  StorageIO(const std::string& filePath,
            Mode fileMode,
            unsigned int numPlanes = 0,
            const unsigned int treeMask = 0,
            const std::vector<bool>* planeMask = 0);
  /** Destructor */
  ~StorageIO();

  uint64_t getNumEvents() const { return _numEvents; }
  unsigned int getNumPlanes() const { return _numPlanes; }
  Storage::Mode getMode() const { return _fileMode; }

  /** Read the event in-place. Replaces all existing event content. */
  void readEvent(uint64_t n, Event* event);
  Event* readEvent(uint64_t n);  // Read an event and generate its objects
  void writeEvent(Event* event); // Write an event at the end of the file

private:
  StorageIO(const StorageIO&);            // Disable the copy constructor
  StorageIO& operator=(const StorageIO&); // Disable the assignment operator
  void openRead(const std::string& path, const std::vector<bool>* planeMask);
  void openTruncate(const std::string& path);
  void clearVariables();

private:
  TFile* _file;            // Storage file
  const Mode _fileMode;    // How to open and process the file
  unsigned int _numPlanes; // This can be read from the file structure
  uint64_t _numEvents;     // Number of events in the input file

  /* NOTE: trees can easily be added and removed from a file. So each type
   * of information that might or might not be included in a file should be
   * in its own tree. */

  // Trees containing event-by-event data for each plane
  std::vector<TTree*> _hits;
  std::vector<TTree*> _clusters;
  std::vector<TTree*> _intercepts;
  // Trees global to the entire event
  TTree* _tracks;
  TTree* _eventInfo;

  //
  // Variables in which the storage is output on an event-by-event basis
  //
  // HITS
  Int_t numHits;
  Int_t hitPixX[MAX_HITS];
  Int_t hitPixY[MAX_HITS];
  Int_t hitValue[MAX_HITS];
  Int_t hitTiming[MAX_HITS];
  Int_t hitInCluster[MAX_HITS];

  // CLUSTERS
  Int_t numClusters;
  Double_t clusterPixX[MAX_CLUSTERS];
  Double_t clusterPixY[MAX_CLUSTERS];
  Double_t clusterPixErrX[MAX_CLUSTERS];
  Double_t clusterPixErrY[MAX_CLUSTERS];
  Int_t clusterInTrack[MAX_CLUSTERS];

  // Local track states
  Int_t numIntercepts;
  Double_t interceptU[MAX_TRACKS];
  Double_t interceptV[MAX_TRACKS];
  Double_t interceptSlopeU[MAX_TRACKS];
  Double_t interceptSlopeV[MAX_TRACKS];
  Double_t interceptStdU[MAX_TRACKS];
  Double_t interceptStdV[MAX_TRACKS];
  Double_t interceptStdSlopeU[MAX_TRACKS];
  Double_t interceptStdSlopeV[MAX_TRACKS];
  Int_t interceptTrack[MAX_TRACKS];

  // EVENT INFO
  ULong64_t timestamp;
  Int_t triggerOffset;
  ULong64_t frameNumber;
  Int_t triggerInfo;
  Int_t triggerPhase;
  Bool_t invalid;

  // TRACKS
  Int_t numTracks;
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
  TBranch* bInterceptTrack;
  TBranch* bInterceptU;
  TBranch* bInterceptV;
  TBranch* bInterceptSlopeU;
  TBranch* bInterceptSlopeV;
  TBranch* bInterceptStdU;
  TBranch* bInterceptStdV;
  TBranch* bInterceptStdSlopeU;
  TBranch* bInterceptStdSlopeV;

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
};

} // end of namespace

#endif // PT_STORAGEIO_H
