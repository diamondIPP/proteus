#include "rceroot.h"

#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(RceRoot)

// -----------------------------------------------------------------------------
// common

Io::RceRootCommon::RceRootCommon(TFile* file)
    : m_file(file)
    , m_entries(0)
    , m_next(0)
    , m_eventInfo(nullptr)
    , m_tracks(nullptr)
{
}

// -----------------------------------------------------------------------------
// reader

bool Io::RceRootReader::isValid(const std::string& path)
{
  std::unique_ptr<TFile> file(TFile::Open(path.c_str(), "READ"));
  TTree* event = nullptr;

  if (!file)
    return false;
  // Minimal file must have at least the Event tree
  file->GetObject("Event", event);
  if (!event)
    return false;
  // readable file + event tree -> probably an RCE ROOT file
  return true;
}

Io::RceRootReader::RceRootReader(const std::string& path)
    : RceRootCommon(TFile::Open(path.c_str(), "READ"))
{
  if (!m_file)
    FAIL("could not open '", path, "' to read");

  // event tree **must** be available
  m_file->GetObject("Event", m_eventInfo);
  if (!m_eventInfo)
    FAIL("could not setup 'Event' tree from '", path, "'");
  m_eventInfo->SetBranchAddress("FrameNumber", &frameNumber);
  m_eventInfo->SetBranchAddress("TimeStamp", &timestamp);
  m_eventInfo->SetBranchAddress("TriggerTime", &triggerTime);
  m_eventInfo->SetBranchAddress("TriggerInfo", &triggerInfo);
  m_eventInfo->SetBranchAddress("TriggerOffset", &triggerOffset);
  // trigger phase is a custom addition for the trigger phase busy and
  // might not be available
  if (m_eventInfo->FindBranch("TriggerPhase")) {
    m_eventInfo->SetBranchAddress("TriggerPhase", &triggerPhase);
  } else {
    triggerPhase = -1;
  }
  m_eventInfo->SetBranchAddress("Invalid", &invalid);

  // tracks tree is optional
  m_file->GetObject("Tracks", m_tracks);
  if (m_tracks) {
    m_tracks->SetBranchAddress("NTracks", &numTracks);
    m_tracks->SetBranchAddress("Chi2", trackChi2);
    m_tracks->SetBranchAddress("Dof", trackDof);
    m_tracks->SetBranchAddress("X", trackX);
    m_tracks->SetBranchAddress("Y", trackY);
    m_tracks->SetBranchAddress("SlopeX", trackSlopeX);
    m_tracks->SetBranchAddress("SlopeY", trackSlopeY);
    m_tracks->SetBranchAddress("Cov", trackCov);
  }

  // per-sensor trees
  size_t numSensors = 0;
  while (true) {
    std::string name("Plane" + std::to_string(numSensors));
    TDirectory* sensorDir = nullptr;
    m_file->GetObject(name.c_str(), sensorDir);
    if (!sensorDir)
      break;
    addSensor(sensorDir);
    numSensors += 1;
  }
  INFO("read ", numSensors, " sensors from '", path, "'");

  // verify that all trees have consistent number of entries
  m_entries = m_eventInfo->GetEntriesFast();
  if (m_entries < 0)
    FAIL("could not determine number of entries");

  auto hasConsistentEntries = [&](TTree* tree) {
    int64_t entries = tree->GetEntriesFast();
    if (entries < 0)
      FAIL("could not determine number of entries");
    return (entries == m_entries);
  };

  // tracks must be consistent with events
  if (m_tracks && !hasConsistentEntries(m_tracks))
    FAIL("inconsistent 'Tracks' entries");
  // per-sensor trees should be consistent, but Mimosa26 trees can have
  // an inconsistent number of entries. ignore those cases
  for (size_t isensor = 0; isensor < m_sensors.size(); ++isensor) {
    auto& trees = m_sensors[isensor];
    if (trees.hits && !hasConsistentEntries(trees.hits)) {
      INFO("ignore sensor ", isensor, " 'Hits' due to inconsistent entries");
      trees.hits = nullptr;
    }
    if (trees.clusters && !hasConsistentEntries(trees.clusters)) {
      INFO("ignore sensor ", isensor,
           " 'Clusters' due to inconsistent entries");
      trees.clusters = nullptr;
    }
    if (trees.intercepts && !hasConsistentEntries(trees.intercepts)) {
      INFO("ignore sensor ", isensor,
           " 'Intercepts' due to inconsistent entries");
      trees.intercepts = nullptr;
    }
  }
}

void Io::RceRootReader::addSensor(TDirectory* dir)
{
  SensorTrees trees;

  dir->GetObject("Hits", trees.hits);
  if (trees.hits) {
    trees.hits->SetBranchAddress("NHits", &numHits);
    trees.hits->SetBranchAddress("PixX", hitPixX);
    trees.hits->SetBranchAddress("PixY", hitPixY);
    trees.hits->SetBranchAddress("Timing", hitTiming);
    trees.hits->SetBranchAddress("Value", hitValue);
    trees.hits->SetBranchAddress("HitInCluster", hitInCluster);
  }
  dir->GetObject("Clusters", trees.clusters);
  if (trees.clusters) {
    trees.clusters->SetBranchAddress("NClusters", &numClusters);
    trees.clusters->SetBranchAddress("Col", clusterCol);
    trees.clusters->SetBranchAddress("Row", clusterRow);
    trees.clusters->SetBranchAddress("VarCol", clusterVarCol);
    trees.clusters->SetBranchAddress("VarRow", clusterVarRow);
    trees.clusters->SetBranchAddress("CovColRow", clusterCovColRow);
    trees.clusters->SetBranchAddress("Track", clusterTrack);
  }
  dir->GetObject("Intercepts", trees.intercepts);
  if (trees.intercepts) {
    trees.intercepts->SetBranchAddress("NIntercepts", &numIntercepts);
    trees.intercepts->SetBranchAddress("U", interceptU);
    trees.intercepts->SetBranchAddress("V", interceptV);
    trees.intercepts->SetBranchAddress("SlopeU", interceptSlopeU);
    trees.intercepts->SetBranchAddress("SlopeV", interceptSlopeV);
    trees.intercepts->SetBranchAddress("Cov", interceptCov);
    trees.intercepts->SetBranchAddress("Track", interceptTrack);
  }
  m_sensors.push_back(trees);
}

Io::RceRootReader::~RceRootReader()
{
  if (m_file) {
    m_file->Close();
    delete m_file;
  }
}

std::string Io::RceRootReader::name() const { return "RceRootReader"; }

uint64_t Io::RceRootReader::numEvents() const
{
  return static_cast<uint64_t>(m_entries);
}

void Io::RceRootReader::skip(uint64_t n)
{
  if (m_entries <= static_cast<int64_t>(m_next + n)) {
    INFO("skipping ", n, " events goes beyond available events");
    m_next = m_entries;
  } else {
    m_next += n;
  }
}

//=========================================================
bool Io::RceRootReader::read(Storage::Event& event)
{
  /* Note: fill in reversed order: tracks first, hits last. This is so that
   * once a hit is produced, it can immediately recieve the address of its
   * parent cluster, likewise for clusters and track. */

  if (m_entries <= m_next)
    return false;

  int64_t ievent = m_next++;

  // global event data
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
  event.setTrigger(triggerInfo, triggerOffset, triggerPhase);

  // global tracks info
  if (m_tracks) {
    if (m_tracks->GetEntry(ievent) <= 0)
      FAIL("could not read 'Tracks' entry ", ievent);
    for (Int_t itrack = 0; itrack < numTracks; ++itrack) {
      Storage::TrackState state(trackX[itrack], trackY[itrack],
                                trackSlopeX[itrack], trackSlopeY[itrack]);
      state.setCov(trackCov[itrack]);
      std::unique_ptr<Storage::Track> track(new Storage::Track(state));
      track->setGoodnessOfFit(trackChi2[itrack], trackDof[itrack]);
      event.addTrack(std::move(track));
    }
  }

  // per-sensor data
  for (size_t isensor = 0; isensor < numSensors(); ++isensor) {
    SensorTrees& trees = m_sensors[isensor];
    Storage::SensorEvent& sensorEvent = event.getSensorEvent(isensor);

    // local track states
    if (trees.intercepts) {
      if (trees.intercepts->GetEntry(ievent) <= 0)
        FAIL("could not read 'Intercepts' entry ", ievent);

      for (Int_t iintercept = 0; iintercept < numIntercepts; ++iintercept) {
        Storage::TrackState local(
            interceptU[iintercept], interceptV[iintercept],
            interceptSlopeU[iintercept], interceptSlopeV[iintercept]);
        local.setCov(interceptCov[iintercept]);
        sensorEvent.setLocalState(interceptTrack[iintercept], std::move(local));
      }
    }

    // local clusters
    if (trees.clusters) {
      if (trees.clusters->GetEntry(ievent) <= 0)
        FAIL("could not read 'Clusters' entry ", ievent);

      for (Int_t icluster = 0; icluster < numClusters; ++icluster) {
        SymMatrix2 cov;
        cov(0, 0) = clusterVarCol[icluster];
        cov(1, 1) = clusterVarRow[icluster];
        cov(0, 1) = clusterCovColRow[icluster];
        Storage::Cluster& cluster = sensorEvent.addCluster();
        cluster.setPixel(XYPoint(clusterCol[icluster], clusterRow[icluster]),
                         cov);
        // Fix cluster/track relationship if possible
        if (m_tracks && (0 <= clusterTrack[icluster])) {
          Storage::Track& track = event.getTrack(clusterTrack[icluster]);
          track.addCluster(isensor, cluster);
          cluster.setTrack(clusterTrack[icluster]);
        }
      }
    }

    // local hits
    if (trees.hits) {
      if (trees.hits->GetEntry(ievent) <= 0)
        FAIL("could not read 'Hits' entry ", ievent);

      for (Int_t ihit = 0; ihit < numHits; ++ihit) {
        Storage::Hit& hit = sensorEvent.addHit(hitPixX[ihit], hitPixY[ihit],
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

Io::RceRootWriter::RceRootWriter(const std::string& path, size_t numSensors)
    : RceRootCommon(TFile::Open(path.c_str(), "RECREATE"))
{
  if (!m_file)
    FAIL("could not open '", path, "' to write");

  // global event tree
  m_eventInfo = new TTree("Event", "Event information");
  m_eventInfo->Branch("FrameNumber", &frameNumber, "FrameNumber/l");
  m_eventInfo->Branch("TimeStamp", &timestamp, "TimeStamp/l");
  m_eventInfo->Branch("TriggerTime", &triggerTime, "TriggerTime/l");
  m_eventInfo->Branch("TriggerInfo", &triggerInfo, "TriggerInfo/I");
  m_eventInfo->Branch("TriggerOffset", &triggerOffset, "TriggerOffset/I");
  m_eventInfo->Branch("TriggerPhase", &triggerPhase, "TriggerPhase/I");
  m_eventInfo->Branch("Invalid", &invalid, "Invalid/O");

  // global track tree
  m_tracks = new TTree("Tracks", "Track parameters");
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

void Io::RceRootWriter::addSensor(TDirectory* dir)
{
  dir->cd();

  SensorTrees trees;
  // local hits
  trees.hits = new TTree("Hits", "Hits");
  trees.hits->Branch("NHits", &numHits, "NHits/I");
  trees.hits->Branch("PixX", hitPixX, "HitPixX[NHits]/I");
  trees.hits->Branch("PixY", hitPixY, "HitPixY[NHits]/I");
  trees.hits->Branch("Timing", hitTiming, "HitTiming[NHits]/I");
  trees.hits->Branch("Value", hitValue, "HitValue[NHits]/I");
  trees.hits->Branch("HitInCluster", hitInCluster, "HitInCluster[NHits]/I");
  // local clusters
  trees.clusters = new TTree("Clusters", "Clusters");
  trees.clusters->Branch("NClusters", &numClusters, "NClusters/I");
  trees.clusters->Branch("Col", clusterCol, "Col[NClusters]/D");
  trees.clusters->Branch("Row", clusterRow, "Row[NClusters]/D");
  trees.clusters->Branch("VarCol", clusterVarCol, "VarCol[NClusters]/D");
  trees.clusters->Branch("VarRow", clusterVarRow, "VarRow[NClusters]/D");
  trees.clusters->Branch("CovColRow", clusterCovColRow,
                         "CovColRow[NClusters]/D");
  trees.clusters->Branch("Track", clusterTrack, "Track[NClusters]/I");
  // local track states
  trees.intercepts = new TTree("Intercepts", "Intercepts");
  trees.intercepts->Branch("NIntercepts", &numIntercepts, "NIntercepts/I");
  trees.intercepts->Branch("U", interceptU, "U[NIntercepts]/D");
  trees.intercepts->Branch("V", interceptV, "V[NIntercepts]/D");
  trees.intercepts->Branch("SlopeU", interceptSlopeU, "SlopeU[NIntercepts]/D");
  trees.intercepts->Branch("SlopeV", interceptSlopeV, "SlopeV[NIntercepts]/D");
  trees.intercepts->Branch("Cov", interceptCov, "Cov[NIntercepts][10]/D");
  trees.intercepts->Branch("Track", interceptTrack, "Track[NIntercepts]/I");
  m_sensors.emplace_back(trees);
}

Io::RceRootWriter::~RceRootWriter()
{
  if (m_file) {
    INFO("wrote ", m_sensors.size(), " sensors to '", m_file->GetPath(), "'");
    m_file->Write();
    m_file->Close();
    delete m_file;
  }
}

std::string Io::RceRootWriter::name() const { return "RceRootWriter"; }

void Io::RceRootWriter::append(const Storage::Event& event)
{
  if (event.numSensorEvents() != m_sensors.size())
    FAIL("inconsistent sensors numbers. events has ", event.numSensorEvents(),
         ", but the writer expected ", m_sensors.size());

  // global event info is **always** filled
  frameNumber = event.frame();
  timestamp = 0;
  triggerTime = event.timestamp();
  triggerInfo = event.triggerInfo();
  triggerOffset = event.triggerOffset();
  triggerPhase = event.triggerPhase();
  invalid = false;
  m_eventInfo->Fill();

  // tracks
  if (m_tracks) {
    if (kMaxTracks < event.numTracks())
      FAIL("tracks exceed MAX_TRACKS");
    numTracks = event.numTracks();
    for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
      const Storage::Track& track = event.getTrack(itrack);
      trackChi2[itrack] = track.chi2();
      trackDof[itrack] = track.degreesOfFreedom();
      const Storage::TrackState& state = track.globalState();
      trackX[itrack] = state.offset().x();
      trackY[itrack] = state.offset().y();
      trackSlopeX[itrack] = state.slope().x();
      trackSlopeY[itrack] = state.slope().y();
      std::copy(state.cov().begin(), state.cov().end(), trackCov[itrack]);
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
        const Storage::Hit hit = sensorEvent.getHit(ihit);
        hitPixX[ihit] = hit.digitalCol();
        hitPixY[ihit] = hit.digitalRow();
        hitValue[ihit] = hit.value();
        hitTiming[ihit] = hit.time();
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
        const Storage::Cluster& cluster = sensorEvent.getCluster(iclu);
        clusterCol[iclu] = cluster.posPixel().x();
        clusterRow[iclu] = cluster.posPixel().y();
        clusterVarCol[iclu] = cluster.covPixel()(0, 0);
        clusterVarRow[iclu] = cluster.covPixel()(1, 1);
        clusterCovColRow[iclu] = cluster.covPixel()(0, 1);
        clusterTrack[iclu] = cluster.isInTrack() ? cluster.track() : -1;
      }
      trees.clusters->Fill();
    }

    // local track states
    if (trees.intercepts) {
      numIntercepts = 0;
      for (const auto& s : sensorEvent.localStates()) {
        if (kMaxTracks < static_cast<Index>(numIntercepts + 1))
          FAIL("intercepts exceed MAX_TRACKS");
        const Storage::TrackState& local = s.second;
        interceptU[numIntercepts] = local.offset().x();
        interceptV[numIntercepts] = local.offset().y();
        interceptSlopeU[numIntercepts] = local.slope().x();
        interceptSlopeV[numIntercepts] = local.slope().y();
        std::copy(local.cov().begin(), local.cov().end(),
                  interceptCov[numIntercepts]);
        interceptTrack[numIntercepts] = s.first;
        numIntercepts += 1;
      }
      trees.intercepts->Fill();
    }
  }
  m_entries += 1;
}
