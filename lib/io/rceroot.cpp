#include "rceroot.h"

#include <cassert>

#include "Compression.h"

#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(RceRoot)

namespace proteus {

// -----------------------------------------------------------------------------
// common

RceRootCommon::RceRootCommon(RootFilePtr&& file)
    : m_file(std::move(file))
    , m_entries(0)
    , m_next(0)
    , m_eventInfo(nullptr)
    , m_tracks(nullptr)
{
}

// -----------------------------------------------------------------------------
// reader

int RceRootReader::check(const std::string& path)
{
  std::unique_ptr<TFile> file(TFile::Open(path.c_str(), "READ"));
  if (!file)
    return 0;

  int score = 0;
  // should have an event tree, but is sometimes missing
  if (file->GetObjectUnchecked("Event"))
    score += 50;
  // should have at least one sensor directory
  if (file->GetObjectUnchecked("Plane0"))
    score += 50;
  return score;
}

std::shared_ptr<RceRootReader>
RceRootReader::open(const std::string& path,
                    const toml::Value& /* unused configuration */)
{
  return std::make_shared<RceRootReader>(path);
}

RceRootReader::RceRootReader(const std::string& path)
    : RceRootCommon(openRootRead(path.c_str()))
{
  int64_t entriesEvent = INT64_MAX;
  int64_t entriesTracks = INT64_MAX;

  // event tree is optional
  m_file->GetObject("Event", m_eventInfo);
  if (m_eventInfo) {
    entriesEvent = m_eventInfo->GetEntriesFast();
    if (entriesEvent < 0)
      THROW("could not determine number of entries of Event tree");
    m_eventInfo->SetBranchAddress("FrameNumber", &frameNumber);
    m_eventInfo->SetBranchAddress("TimeStamp", &timestamp);
    m_eventInfo->SetBranchAddress("TriggerTime", &triggerTime);
    m_eventInfo->SetBranchAddress("Invalid", &invalid);
  }

  // tracks tree is optional
  m_file->GetObject("Tracks", m_tracks);
  if (m_tracks) {
    entriesTracks = m_tracks->GetEntriesFast();
    if (entriesTracks < 0)
      THROW("could not determine number of entries in Tracks tree");
    m_tracks->SetBranchAddress("NTracks", &numTracks);
    m_tracks->SetBranchAddress("Chi2", trackChi2);
    m_tracks->SetBranchAddress("Dof", trackDof);
    m_tracks->SetBranchAddress("X", trackX);
    m_tracks->SetBranchAddress("Y", trackY);
    m_tracks->SetBranchAddress("SlopeX", trackSlopeX);
    m_tracks->SetBranchAddress("SlopeY", trackSlopeY);
    m_tracks->SetBranchAddress("Cov", trackCov);
  }

  // entries from Events and Tracks. might still be undefined here
  m_entries = std::min(entriesEvent, entriesTracks);

  // per-sensor trees and finalize number of entries
  size_t numSensors = 0;
  while (true) {
    std::string name("Plane" + std::to_string(numSensors));
    TDirectory* sensorDir = nullptr;
    m_file->GetObject(name.c_str(), sensorDir);
    if (!sensorDir)
      break;
    int64_t entriesSensor = addSensor(sensorDir);
    m_entries = std::min(m_entries, entriesSensor);
    numSensors += 1;
  }
  if (numSensors == 0)
    THROW("no sensors in '", path, "'");
  if (m_entries == INT64_MAX)
    THROW("could not determine number of events in '", path, "'");
  INFO("read ", numSensors, " sensors from '", path, "'");

  // NOTE 2017-10-25 msmk:
  //
  // having inconsistent entries between different sensors and the global
  // trees should be a fatal error. unfortunately, this can still happen for
  // valid data, e.g. for telescope data w/ manually synced trigger/busy-based
  // dut data or for independent Mimosa26 streams. To be able to handle these
  // we only report these cases as errrors here instead of failing altogether.

  // verify consistent number of entries between all trees
  if ((entriesEvent != INT64_MAX) && (entriesEvent != m_entries))
    ERROR("Event tree has inconsistent entries=", entriesEvent,
          " expected=", m_entries);
  if ((entriesTracks != INT64_MAX) && (entriesTracks != m_entries))
    ERROR("Tracks tree has inconsistent entries=", entriesTracks,
          " expected=", m_entries);
  for (size_t isensor = 0; isensor < m_sensors.size(); ++isensor) {
    if (m_sensors[isensor].entries != m_entries)
      ERROR("sensor ", isensor,
            " has inconsistent entries=", m_sensors[isensor].entries,
            " expected=", m_entries);
  }
}

/** Setup trees for a new sensor and return the number of entries.
 *
 * Throws on inconsistent number of entries.
 */
int64_t RceRootReader::addSensor(TDirectory* dir)
{
  assert(dir && "Directory must be non-null");

  SensorTrees trees;
  // use INT64_MAX to mark uninitialized/ missing values that can be used
  // directly in std::min to find the number of entries
  int64_t entriesHits = INT64_MAX;
  int64_t entriesClusters = INT64_MAX;
  int64_t entriesIntercepts = INT64_MAX;

  dir->GetObject("Hits", trees.hits);
  if (trees.hits) {
    entriesHits = trees.hits->GetEntriesFast();
    if (entriesHits < 0)
      THROW("could not determine entries in ", dir->GetName(), "/Hits tree");
    trees.hits->SetBranchAddress("NHits", &numHits);
    trees.hits->SetBranchAddress("PixX", hitPixX);
    trees.hits->SetBranchAddress("PixY", hitPixY);
    trees.hits->SetBranchAddress("Timing", hitTiming);
    trees.hits->SetBranchAddress("Value", hitValue);
    trees.hits->SetBranchAddress("HitInCluster", hitInCluster);
  }
  dir->GetObject("Clusters", trees.clusters);
  if (trees.clusters) {
    entriesClusters = trees.clusters->GetEntriesFast();
    if (entriesClusters < 0)
      THROW("could not determine entries in ", dir->GetName(),
            "/Clusters tree");
    trees.clusters->SetBranchAddress("NClusters", &numClusters);
    trees.clusters->SetBranchAddress("Col", clusterCol);
    trees.clusters->SetBranchAddress("Row", clusterRow);
    trees.clusters->SetBranchAddress("VarCol", clusterVarCol);
    trees.clusters->SetBranchAddress("VarRow", clusterVarRow);
    trees.clusters->SetBranchAddress("Timing", clusterTiming);
    trees.clusters->SetBranchAddress("Value", clusterValue);
    trees.clusters->SetBranchAddress("Track", clusterTrack);
  }
  dir->GetObject("Intercepts", trees.intercepts);
  if (trees.intercepts) {
    entriesIntercepts = trees.hits->GetEntriesFast();
    if (entriesIntercepts < 0)
      THROW("could not determine entries in ", dir->GetName(),
            "Intercepts tree");
    trees.intercepts->SetBranchAddress("NIntercepts", &numIntercepts);
    trees.intercepts->SetBranchAddress("U", interceptU);
    trees.intercepts->SetBranchAddress("V", interceptV);
    trees.intercepts->SetBranchAddress("SlopeU", interceptSlopeU);
    trees.intercepts->SetBranchAddress("SlopeV", interceptSlopeV);
    trees.intercepts->SetBranchAddress("Cov", interceptCov);
    trees.intercepts->SetBranchAddress("Track", interceptTrack);
  }

  // this directory does not contain any valid data
  if ((entriesHits == INT64_MAX) && (entriesClusters == INT64_MAX) &&
      (entriesIntercepts == INT64_MAX))
    THROW("could not find any of ", dir->GetName(),
          "/{Hits,Clusters,Intercepts}");

  // check that all active trees have consistent entries
  trees.entries = std::min({entriesHits, entriesClusters, entriesIntercepts});
  if ((entriesHits != INT64_MAX) && (entriesHits != trees.entries))
    THROW("inconsistent entries in ", dir->GetName(),
          "/Hits tree entries=", entriesHits, " expected=", trees.entries);
  if ((entriesClusters != INT64_MAX) && (entriesClusters != trees.entries))
    THROW("inconsistent entries in ", dir->GetName(),
          "/Clusters tree entries=", entriesClusters,
          " expected=", trees.entries);
  if ((entriesIntercepts != INT64_MAX) && (entriesIntercepts != trees.entries))
    THROW("inconsistent entries in ", dir->GetName(),
          "/Intercepts tree entries=", entriesIntercepts,
          " expected=", trees.entries);

  m_sensors.push_back(trees);
  return trees.entries;
}

std::string RceRootReader::name() const { return "RceRootReader"; }

uint64_t RceRootReader::numEvents() const
{
  return static_cast<uint64_t>(m_entries);
}

size_t RceRootReader::numSensors() const { return m_sensors.size(); }

void RceRootReader::skip(uint64_t n)
{
  if (m_entries <= static_cast<int64_t>(m_next + n)) {
    ERROR("skipping ", n, " events goes beyond available events");
    m_next = m_entries;
  } else {
    m_next += n;
  }
}

bool RceRootReader::read(Event& event)
{
  /* Note: fill in reversed order: tracks first, hits last. This is so that
   * once a hit is produced, it can immediately recieve the address of its
   * parent cluster, likewise for clusters and track. */

  if (m_entries <= m_next)
    return false;

  int64_t ievent = m_next++;

  // global event data
  if (m_eventInfo) {
    if (m_eventInfo->GetEntry(ievent) <= 0)
      FAIL("could not read 'Events' entry ", ievent);
    // listen chap, here's the deal:
    // we want a timestamp, i.e. a simple counter of clockcycles or bunch
    // crossings, for each event that defines the trigger/ readout time with
    // the highest possible precision. Unfortunately, the RCE ROOT output format
    // has stupid names. The `TimeStamp` branch stores the Unix-`timestamp`
    // (number of seconds since 01.01.1970) of the point in time when the event
    // was written to disk. This might or might not have a constant correlation
    // to the actual trigger time and has only a 1s resolution, i.e. it is
    // completely useless. The `TriggerTime` actually stores the internal
    // FPGA timestamp/ clock cyles and is what we need to use.
    event.clear(frameNumber, triggerTime);
  } else {
    event.clear(ievent);
  }

  // global tracks info
  if (m_tracks) {
    if (m_tracks->GetEntry(ievent) <= 0)
      FAIL("could not read 'Tracks' entry ", ievent);
    for (Int_t itrack = 0; itrack < numTracks; ++itrack) {
      TrackState state(trackX[itrack], trackY[itrack], trackSlopeX[itrack],
                       trackSlopeY[itrack]);
      state.setCovSpatialPacked(trackCov[itrack]);
      event.addTrack({state, trackChi2[itrack], trackDof[itrack]});
    }
  }

  // per-sensor data
  for (size_t isensor = 0; isensor < numSensors(); ++isensor) {
    SensorTrees& trees = m_sensors[isensor];
    SensorEvent& sensorEvent = event.getSensorEvent(isensor);

    // local track states
    if (trees.intercepts) {
      if (trees.intercepts->GetEntry(ievent) <= 0)
        FAIL("could not read 'Intercepts' entry ", ievent);

      for (Int_t iintercept = 0; iintercept < numIntercepts; ++iintercept) {
        TrackState local(interceptU[iintercept], interceptV[iintercept],
                         interceptSlopeU[iintercept],
                         interceptSlopeV[iintercept]);
        local.setCovSpatialPacked(interceptCov[iintercept]);
        sensorEvent.setLocalState(interceptTrack[iintercept], local);
      }
    }

    // local clusters
    if (trees.clusters) {
      if (trees.clusters->GetEntry(ievent) <= 0)
        FAIL("could not read 'Clusters' entry ", ievent);

      for (Int_t icluster = 0; icluster < numClusters; ++icluster) {
        Cluster& cluster = sensorEvent.addCluster(
            clusterCol[icluster], clusterRow[icluster], clusterTiming[icluster],
            clusterValue[icluster], clusterVarCol[icluster],
            clusterVarRow[icluster], 1.0 / 12.0, clusterCovColRow[icluster]);
        // Fix cluster/track relationship if possible
        if (m_tracks && (0 <= clusterTrack[icluster])) {
          cluster.setTrack(clusterTrack[icluster]);
          event.getTrack(clusterTrack[icluster]).addCluster(isensor, icluster);
        }
      }
    }

    // local hits
    if (trees.hits) {
      if (trees.hits->GetEntry(ievent) <= 0)
        FAIL("could not read 'Hits' entry ", ievent);

      for (Int_t ihit = 0; ihit < numHits; ++ihit) {
        Hit& hit = sensorEvent.addHit(hitPixX[ihit], hitPixY[ihit],
                                      hitTiming[ihit], hitValue[ihit]);
        // Fix hit/cluster relationship is possibl
        if (trees.clusters && hitInCluster[ihit] >= 0)
          sensorEvent.getCluster(hitInCluster[ihit]).addHit(hit);
      }
    }
  } // end loop in planes
  return true;
}

// -----------------------------------------------------------------------------
// writer

RceRootWriter::RceRootWriter(const std::string& path, size_t numSensors)
    : RceRootCommon(openRootWrite(path))
{
  m_file->cd();

  // global event tree
  m_eventInfo = new TTree("Event", "Event information");
  m_eventInfo->SetDirectory(m_file.get());
  m_eventInfo->Branch("FrameNumber", &frameNumber, "FrameNumber/l");
  m_eventInfo->Branch("TimeStamp", &timestamp, "TimeStamp/l");
  m_eventInfo->Branch("TriggerTime", &triggerTime, "TriggerTime/l");
  m_eventInfo->Branch("Invalid", &invalid, "Invalid/O");

  // global track tree
  m_tracks = new TTree("Tracks", "Track parameters");
  m_tracks->SetDirectory(m_file.get());
  m_tracks->Branch("NTracks", &numTracks, "NTracks/I");
  m_tracks->Branch("Chi2", trackChi2, "Chi2[NTracks]/D");
  m_tracks->Branch("Dof", trackDof, "Dof[NTracks]/I");
  m_tracks->Branch("X", trackX, "X[NTracks]/D");
  m_tracks->Branch("Y", trackY, "Y[NTracks]/D");
  m_tracks->Branch("SlopeX", trackSlopeX, "SlopeX[NTracks]/D");
  m_tracks->Branch("SlopeY", trackSlopeY, "SlopeY[NTracks]/D");
  m_tracks->Branch("Cov", trackCov, "Cov[NTracks][10]/D");

  // per-sensor trees
  for (size_t isensor = 0; isensor < numSensors; ++isensor) {
    std::string name("Plane" + std::to_string(isensor));
    TDirectory* sensorDir = m_file->mkdir(name.c_str());
    addSensor(sensorDir);
  }
}

void RceRootWriter::addSensor(TDirectory* dir)
{
  dir->cd();

  SensorTrees trees;
  // local hits
  trees.hits = new TTree("Hits", "Hits");
  trees.hits->SetDirectory(dir);
  trees.hits->Branch("NHits", &numHits, "NHits/I");
  trees.hits->Branch("PixX", hitPixX, "HitPixX[NHits]/I");
  trees.hits->Branch("PixY", hitPixY, "HitPixY[NHits]/I");
  trees.hits->Branch("Timing", hitTiming, "HitTiming[NHits]/I");
  trees.hits->Branch("Value", hitValue, "HitValue[NHits]/I");
  trees.hits->Branch("HitInCluster", hitInCluster, "HitInCluster[NHits]/I");
  // local clusters
  trees.clusters = new TTree("Clusters", "Clusters");
  trees.clusters->SetDirectory(dir);
  trees.clusters->Branch("NClusters", &numClusters, "NClusters/I");
  trees.clusters->Branch("Col", clusterCol, "Col[NClusters]/D");
  trees.clusters->Branch("Row", clusterRow, "Row[NClusters]/D");
  trees.clusters->Branch("VarCol", clusterVarCol, "VarCol[NClusters]/D");
  trees.clusters->Branch("VarRow", clusterVarRow, "VarRow[NClusters]/D");
  trees.clusters->Branch("CovColRow", clusterCovColRow,
                         "CovColRow[NClusters]/D");
  trees.clusters->Branch("Timing", clusterTiming, "Timing[NClusters]/D");
  trees.clusters->Branch("Value", clusterValue, "Value[NClusters]/D");
  trees.clusters->Branch("Track", clusterTrack, "Track[NClusters]/I");
  // local track states
  trees.intercepts = new TTree("Intercepts", "Intercepts");
  trees.intercepts->SetDirectory(dir);
  trees.intercepts->Branch("NIntercepts", &numIntercepts, "NIntercepts/I");
  trees.intercepts->Branch("U", interceptU, "U[NIntercepts]/D");
  trees.intercepts->Branch("V", interceptV, "V[NIntercepts]/D");
  trees.intercepts->Branch("SlopeU", interceptSlopeU, "SlopeU[NIntercepts]/D");
  trees.intercepts->Branch("SlopeV", interceptSlopeV, "SlopeV[NIntercepts]/D");
  trees.intercepts->Branch("Cov", interceptCov, "Cov[NIntercepts][10]/D");
  trees.intercepts->Branch("Track", interceptTrack, "Track[NIntercepts]/I");
  m_sensors.emplace_back(trees);
}

RceRootWriter::~RceRootWriter()
{
  if (m_file) {
    INFO("wrote ", m_sensors.size(), " sensors to '", m_file->GetPath(), "'");
  }
}

std::string RceRootWriter::name() const { return "RceRootWriter"; }

void RceRootWriter::append(const Event& event)
{
  if (event.numSensorEvents() != m_sensors.size())
    FAIL("inconsistent sensors numbers. events has ", event.numSensorEvents(),
         ", but the writer expected ", m_sensors.size());

  // global event info is **always** filled
  frameNumber = event.frame();
  timestamp = 0;
  triggerTime = event.timestamp();
  invalid = false;
  m_eventInfo->Fill();

  // tracks
  if (m_tracks) {
    if (kMaxTracks < event.numTracks())
      FAIL("tracks exceed MAX_TRACKS");
    numTracks = event.numTracks();
    for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
      const Track& track = event.getTrack(itrack);
      trackChi2[itrack] = track.chi2();
      trackDof[itrack] = track.degreesOfFreedom();
      const TrackState& state = track.globalState();
      trackX[itrack] = state.loc0();
      trackY[itrack] = state.loc1();
      trackSlopeX[itrack] = state.slopeLoc0();
      trackSlopeY[itrack] = state.slopeLoc1();
      state.getCovSpatialPacked(trackCov[itrack]);
    }
    m_tracks->Fill();
  }

  // per-sensor data
  for (Index isensor = 0; isensor < event.numSensorEvents(); ++isensor) {
    SensorTrees& trees = m_sensors[isensor];
    const auto& sensorEvent = event.getSensorEvent(isensor);

    // local hits
    if (trees.hits) {
      if (kMaxHits < sensorEvent.numHits())
        FAIL("hits exceed MAX_HITS");
      numHits = sensorEvent.numHits();
      for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
        const Hit hit = sensorEvent.getHit(ihit);
        hitPixX[ihit] = hit.digitalCol();
        hitPixY[ihit] = hit.digitalRow();
        hitTiming[ihit] = hit.timestamp();
        hitValue[ihit] = hit.value();
        hitInCluster[ihit] = hit.isInCluster() ? hit.cluster() : -1;
      }
      trees.hits->Fill();
    }

    // local clusters
    if (trees.clusters) {
      if (kMaxHits < sensorEvent.numClusters())
        FAIL("clusters exceed MAX_HITS");
      numClusters = sensorEvent.numClusters();
      for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
        const Cluster& cluster = sensorEvent.getCluster(iclu);
        clusterCol[iclu] = cluster.col();
        clusterRow[iclu] = cluster.row();
        clusterVarCol[iclu] = cluster.colVar();
        clusterVarRow[iclu] = cluster.rowVar();
        clusterCovColRow[iclu] = cluster.colRowCov();
        clusterTiming[iclu] = cluster.time();
        clusterValue[iclu] = cluster.value();
        clusterTrack[iclu] = cluster.isInTrack() ? cluster.track() : -1;
      }
      trees.clusters->Fill();
    }

    // local track states
    if (trees.intercepts) {
      numIntercepts = 0;
      for (const auto& local : sensorEvent.localStates()) {
        if (kMaxTracks < static_cast<Index>(numIntercepts + 1))
          FAIL("intercepts exceed MAX_TRACKS");
        interceptU[numIntercepts] = local.loc0();
        interceptV[numIntercepts] = local.loc1();
        interceptSlopeU[numIntercepts] = local.slopeLoc0();
        interceptSlopeV[numIntercepts] = local.slopeLoc1();
        local.getCovSpatialPacked(interceptCov[numIntercepts]);
        interceptTrack[numIntercepts] = local.track();
        numIntercepts += 1;
      }
      trees.intercepts->Fill();
    }
  }
  m_entries += 1;
}

} // namespace proteus
