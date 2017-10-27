#ifndef PT_RCEROOT_H
#define PT_RCEROOT_H

#include <cstdint>
#include <string>
#include <vector>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#include "io/reader.h"
#include "io/writer.h"
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
  static constexpr Index kMaxHits = 1 << 14;
  static constexpr Index kMaxTracks = 1 << 14;

  struct SensorTrees {
    TTree* hits = nullptr;
    TTree* clusters = nullptr;
    TTree* intercepts = nullptr;
    int64_t entries = 0;
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
  Double_t trackChi2[kMaxTracks];
  Int_t trackDof[kMaxTracks];
  Double_t trackX[kMaxTracks];
  Double_t trackY[kMaxTracks];
  Double_t trackSlopeX[kMaxTracks];
  Double_t trackSlopeY[kMaxTracks];
  Double_t trackCov[kMaxTracks][10];
  // local hits
  Int_t numHits;
  Int_t hitPixX[kMaxHits];
  Int_t hitPixY[kMaxHits];
  Int_t hitTiming[kMaxHits];
  Int_t hitValue[kMaxHits];
  Int_t hitInCluster[kMaxHits];
  // local clusters
  Int_t numClusters;
  Double_t clusterCol[kMaxHits];
  Double_t clusterRow[kMaxHits];
  Double_t clusterVarCol[kMaxHits];
  Double_t clusterVarRow[kMaxHits];
  Double_t clusterCovColRow[kMaxHits];
  Double_t clusterTiming[kMaxHits];
  Double_t clusterValue[kMaxHits];
  Int_t clusterTrack[kMaxHits];
  // local track states
  Int_t numIntercepts;
  Double_t interceptU[kMaxTracks];
  Double_t interceptV[kMaxTracks];
  Double_t interceptSlopeU[kMaxTracks];
  Double_t interceptSlopeV[kMaxTracks];
  Double_t interceptCov[kMaxTracks][10];
  Int_t interceptTrack[kMaxTracks];
};

/** Read events from a RCE ROOT file. */
class RceRootReader : public RceRootCommon, public EventReader {
public:
  /** Return a score of how likely the given path is an RCE Root file. */
  static int check(const std::string& path);
  /** Open the the file. */
  static std::shared_ptr<RceRootReader> open(const std::string& path,
                                             const toml::Value& /* unused */);

  /** Open an existing file and determine the number of sensors and events. */
  RceRootReader(const std::string& path);
  ~RceRootReader();

  std::string name() const override final;
  uint64_t numEvents() const override final;
  size_t numSensors() const override final;

  void skip(uint64_t n) override final;
  bool read(Storage::Event& event) override final;

private:
  int64_t addSensor(TDirectory* dir);
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
