#ifndef PT_RCEROOT_H
#define PT_RCEROOT_H

#include <cstdint>
#include <string>
#include <vector>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#include "io.h"
#include "utils/definitions.h"

namespace Io {

/** Common data for RceRoot{Reader,Writer}. */
class RceRootCommon {
protected:
  RceRootCommon(TFile* file);

  /* NOTE: these sizes are used to initialize arrays of track, cluster and
   * hit information. BUT these arrays are generated ONLY ONCE and re-used
   * to load events. Vectors could have been used in the ROOT file format, but
   * they would need to be constructed at each event reading step. */
  static constexpr Index MAX_HITS = 10000;
  static constexpr Index MAX_TRACKS = 10000;

  struct SensorTrees {
    TTree* hits = nullptr;
    TTree* clusters = nullptr;
    TTree* intercepts = nullptr;
  };

  TFile* m_file;
  int64_t m_entries;
  int64_t m_next;
  // Trees global to the entire event
  TTree* m_eventInfo;
  TTree* m_tracks;
  // Trees containing event-by-event data for each sensors
  std::vector<SensorTrees> m_sensors;

  // global event info
  ULong64_t timestamp;
  ULong64_t frameNumber;
  ULong64_t triggerTime;
  Int_t triggerOffset;
  Int_t triggerInfo;
  Int_t triggerPhase;
  Bool_t invalid;
  // global track state and info
  Int_t numTracks;
  Double_t trackChi2[MAX_TRACKS];
  Int_t trackDof[MAX_TRACKS];
  Double_t trackX[MAX_TRACKS];
  Double_t trackY[MAX_TRACKS];
  Double_t trackSlopeX[MAX_TRACKS];
  Double_t trackSlopeY[MAX_TRACKS];
  Double_t trackCov[MAX_TRACKS][10];
  // local hits
  Int_t numHits;
  Int_t hitPixX[MAX_HITS];
  Int_t hitPixY[MAX_HITS];
  Int_t hitTiming[MAX_HITS];
  Int_t hitValue[MAX_HITS];
  Int_t hitInCluster[MAX_HITS];
  // local clusters
  Int_t numClusters;
  Double_t clusterCol[MAX_HITS];
  Double_t clusterRow[MAX_HITS];
  Double_t clusterVarCol[MAX_HITS];
  Double_t clusterVarRow[MAX_HITS];
  Double_t clusterCovColRow[MAX_HITS];
  Int_t clusterTrack[MAX_HITS];
  // local track states
  Int_t numIntercepts;
  Double_t interceptU[MAX_TRACKS];
  Double_t interceptV[MAX_TRACKS];
  Double_t interceptSlopeU[MAX_TRACKS];
  Double_t interceptSlopeV[MAX_TRACKS];
  Double_t interceptCov[MAX_TRACKS][10];
  Int_t interceptTrack[MAX_TRACKS];
};

/** Read events from a RCE ROOT file. */
class RceRootReader : public RceRootCommon, public EventReader {
public:
  /** Open an existing file and determine the number of sensors and events. */
  RceRootReader(const std::string& path);
  ~RceRootReader();

  std::string name() const;
  uint64_t numEvents() const;
  Index numSensors() const { return static_cast<Index>(m_sensors.size()); }

  void skip(uint64_t n);
  bool readNext(Storage::Event& event);

private:
  void addSensor(TDirectory* dir);
};

/** Write event in the RCE ROOT file format. */
class RceRootWriter : public RceRootCommon, public EventWriter {
public:
  /** Open a new file and truncate existing content. */
  RceRootWriter(const std::string& path, size_t numSensors);
  ~RceRootWriter();

  std::string name() const;

  void append(const Storage::Event& event);

private:
  void addSensor(TDirectory* dir);
};

} // namespace Io

#endif // PT_RCEROOT_H
