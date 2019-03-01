#include "match.h"

#include <cassert>
#include <limits>
#include <string>

#include <TDirectory.h>
#include <TTree.h>

#include "mechanics/sensor.h"
#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

void Io::MatchWriter::EventData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("evt_frame", &frame, "evt_frame/l");
  tree->Branch("evt_timestamp", &timestamp, "evt_timestamp/l");
  tree->Branch("evt_nclusters", &nClusters);
  tree->Branch("evt_ntracks", &nTracks);
}

void Io::MatchWriter::EventData::set(const Storage::SensorEvent& e)
{
  frame = e.frame();
  timestamp = e.timestamp();
  nClusters = e.numClusters();
  nTracks = e.localStates().size();
}

void Io::MatchWriter::TrackData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("trk_u", &u);
  tree->Branch("trk_v", &v);
  tree->Branch("trk_time", &time);
  tree->Branch("trk_du", &du);
  tree->Branch("trk_dv", &dv);
  tree->Branch("trk_dtime", &dtime);
  tree->Branch("trk_std_u", &stdU);
  tree->Branch("trk_std_v", &stdV);
  tree->Branch("trk_std_time", &stdTime);
  tree->Branch("trk_corr_uv", &corrUV);
  tree->Branch("trk_col", &col);
  tree->Branch("trk_row", &row);
  tree->Branch("trk_timestamp", &timestamp);
  tree->Branch("trk_chi2", &chi2);
  tree->Branch("trk_dof", &dof);
  tree->Branch("trk_size", &size);
}

void Io::MatchWriter::TrackData::set(const Storage::Track& track,
                                     const Storage::TrackState& state,
                                     const Vector4& posPixel)
{
  u = state.loc0();
  v = state.loc1();
  time = state.time();
  du = state.slopeLoc0();
  dv = state.slopeLoc1();
  dtime = state.slopeTime();
  stdU = std::sqrt(state.cov()(kLoc0, kLoc0));
  stdV = std::sqrt(state.cov()(kLoc1, kLoc1));
  stdTime = std::sqrt(state.timeVar());
  corrUV = state.cov()(kLoc0, kLoc1) / (stdU * stdV);
  col = posPixel[kU];
  row = posPixel[kV];
  timestamp = posPixel[kS];
  chi2 = track.chi2();
  dof = track.degreesOfFreedom();
  size = track.size();
}

void Io::MatchWriter::ClusterData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("clu_u", &u);
  tree->Branch("clu_v", &v);
  tree->Branch("clu_time", &time);
  tree->Branch("clu_std_u", &stdU);
  tree->Branch("clu_std_v", &stdV);
  tree->Branch("clu_std_time", &stdTime);
  tree->Branch("clu_corr_uv", &corrUV);
  tree->Branch("clu_col", &col);
  tree->Branch("clu_row", &row);
  tree->Branch("clu_timestamp", &timestamp);
  tree->Branch("clu_value", &value);
  tree->Branch("clu_region", &region);
  tree->Branch("clu_size", &size);
  tree->Branch("clu_size_col", &sizeCol);
  tree->Branch("clu_size_row", &sizeRow);
  tree->Branch("hit_col", &hitCol, "hit_col[clu_size]/S");
  tree->Branch("hit_row", &hitRow, "hit_row[clu_size]/S");
  tree->Branch("hit_timestamp", &hitTimestamp, "hit_timestamp[clu_size]/S");
  tree->Branch("hit_value", &hitValue, "hit_value[clu_size]/S");
}

void Io::MatchWriter::ClusterData::set(const Storage::Cluster& cluster)
{
  u = cluster.u();
  v = cluster.v();
  time = cluster.time();
  stdU = std::sqrt(cluster.uvCov()(0, 0));
  stdV = std::sqrt(cluster.uvCov()(1, 1));
  stdTime = std::sqrt(cluster.timeVar());
  corrUV = cluster.uvCov()(0, 1) / (stdU * stdV);
  col = cluster.col();
  row = cluster.row();
  timestamp = cluster.timestamp();
  value = cluster.value();
  region = (cluster.hasRegion() ? cluster.region() : -1);
  size = std::min(cluster.size(), size_t(kMaxClusterSize));
  sizeCol = cluster.sizeCol();
  sizeRow = cluster.sizeRow();
  const auto& hits = cluster.hits();
  for (int16_t ihit = 0; ihit < size; ++ihit) {
    const Storage::Hit& hit = hits[ihit];
    hitCol[ihit] = hit.col();
    hitRow[ihit] = hit.row();
    hitTimestamp[ihit] = hit.timestamp();
    hitValue[ihit] = hit.value();
  }
}

void Io::MatchWriter::ClusterData::invalidate()
{
  u = std::numeric_limits<float>::quiet_NaN();
  v = std::numeric_limits<float>::quiet_NaN();
  time = std::numeric_limits<float>::quiet_NaN();
  stdU = std::numeric_limits<float>::quiet_NaN();
  stdV = std::numeric_limits<float>::quiet_NaN();
  corrUV = std::numeric_limits<float>::quiet_NaN();
  col = std::numeric_limits<float>::quiet_NaN();
  row = std::numeric_limits<float>::quiet_NaN();
  value = std::numeric_limits<float>::quiet_NaN();
  timestamp = std::numeric_limits<float>::quiet_NaN();
  region = -1;
  size = 0; // required to have empty hit information
  sizeCol = 0;
  sizeRow = 0;
}

void Io::MatchWriter::MaskData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("col", &col);
  tree->Branch("row", &row);
}

void Io::MatchWriter::DistData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("mat_d2", &d2);
}

Io::MatchWriter::MatchWriter(TDirectory* dir, const Mechanics::Sensor& sensor)
    : m_sensor(sensor)
    , m_sensorId(sensor.id())
    , m_name("MatchWriter(" + sensor.name() + ')')
{
  TDirectory* sub = dir->mkdir(m_sensor.name().c_str());
  sub->cd();

  m_matchedTree = new TTree("tracks_clusters_matched", "");
  m_matchedTree->SetDirectory(sub);
  m_event.addToTree(m_matchedTree);
  m_track.addToTree(m_matchedTree);
  m_matchedCluster.addToTree(m_matchedTree);
  m_matchedDist.addToTree(m_matchedTree);

  m_unmatchTree = new TTree("clusters_unmatched", "");
  m_unmatchTree->SetDirectory(sub);
  m_event.addToTree(m_unmatchTree);
  m_unmatchCluster.addToTree(m_unmatchTree);

  // pixel masks
  TTree* treeMask = new TTree("masked_pixels", "");
  treeMask->SetDirectory(sub);
  MaskData maskData;
  maskData.addToTree(treeMask);
  const auto& mask = m_sensor.pixelMask();
  const auto& cols = m_sensor.colRange();
  const auto& rows = m_sensor.rowRange();
  for (auto c = cols.min(); c < cols.max(); ++c) {
    for (auto r = rows.min(); r < rows.max(); ++r) {
      if (mask.isMasked(c, r)) {
        maskData.col = static_cast<int16_t>(c);
        maskData.row = static_cast<int16_t>(r);
        treeMask->Fill();
      }
    }
  }
}

std::string Io::MatchWriter::name() const { return m_name; }

void Io::MatchWriter::append(const Storage::Event& event)
{
  const auto& sensorEvent = event.getSensorEvent(m_sensorId);

  m_event.set(sensorEvent);

  // export tracks and possible matched clusters
  for (const auto& s : sensorEvent.localStates()) {
    const Storage::TrackState& state = s.second;
    const Storage::Track& track = event.getTrack(s.first);

    // always export track data
    m_track.set(track, state, m_sensor.transformLocalToPixel(state.position()));

    // export matched cluster data if it exists
    if (state.isMatched()) {
      const Storage::Cluster& cluster =
          sensorEvent.getCluster(state.matchedCluster());
      // set cluster information
      m_matchedCluster.set(cluster);
      // set matching information
      Vector2 delta(cluster.u() - state.loc0(), cluster.v() - state.loc1());
      SymMatrix2 cov = cluster.uvCov() + state.loc01Cov();
      m_matchedDist.d2 = mahalanobisSquared(cov, delta);
    } else {
      // fill invalid data if no matching cluster exists
      m_matchedCluster.invalidate();
      m_matchedDist.d2 = std::numeric_limits<float>::quiet_NaN();
    }
    m_matchedTree->Fill();
  }

  // export unmatched clusters
  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = sensorEvent.getCluster(icluster);

    // already exported during track iteration
    if (cluster.isMatched())
      continue;

    m_unmatchCluster.set(cluster);
    m_unmatchTree->Fill();
  }
}
