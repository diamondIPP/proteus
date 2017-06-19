#include "matchexporter.h"

#include <limits>
#include <string>

#include <TDirectory.h>
#include <TTree.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Analyzers::MatchExporter::MatchExporter(const Mechanics::Device& device,
                                        Index sensorId,
                                        TDirectory* dir)
    : m_sensor(*device.getSensor(sensorId))
    , m_sensorId(sensorId)
    , m_name("MatchExporter(" + device.getSensor(sensorId)->name() + ')')
{
  TDirectory* sub = dir->mkdir(m_sensor.name().c_str());

  auto setupCluster = [](TTree* tree, ClusterData& data) {
    tree->Branch("clu_u", &data.u);
    tree->Branch("clu_v", &data.v);
    tree->Branch("clu_std_u", &data.stdU);
    tree->Branch("clu_std_v", &data.stdV);
    tree->Branch("clu_corr_uv", &data.corrUV);
    tree->Branch("clu_col", &data.col);
    tree->Branch("clu_row", &data.row);
    tree->Branch("clu_time", &data.time);
    tree->Branch("clu_value", &data.value);
    tree->Branch("clu_region", &data.region);
    tree->Branch("clu_size", &data.size);
    tree->Branch("clu_size_col", &data.sizeCol);
    tree->Branch("clu_size_row", &data.sizeRow);
    tree->Branch("hit_col", &data.hitCol, "hit_col[clu_size]/S");
    tree->Branch("hit_row", &data.hitRow, "hit_row[clu_size]/S");
    tree->Branch("hit_time", &data.hitTime, "hit_time[clu_size]/F");
    tree->Branch("hit_value", &data.hitValue, "hit_value[clu_size]/F");
  };

  m_treeTrk = new TTree("tracks", "");
  m_treeTrk->SetDirectory(sub);
  m_treeTrk->Branch("evt_timestamp", &m_event.timestamp, "evt_timestamp/l");
  m_treeTrk->Branch("evt_nclusters", &m_event.nClusters);
  m_treeTrk->Branch("evt_ntracks", &m_event.nTracks);
  m_treeTrk->Branch("trk_u", &m_track.u);
  m_treeTrk->Branch("trk_v", &m_track.v);
  m_treeTrk->Branch("trk_du", &m_track.du);
  m_treeTrk->Branch("trk_dv", &m_track.dv);
  m_treeTrk->Branch("trk_std_u", &m_track.stdU);
  m_treeTrk->Branch("trk_std_v", &m_track.stdV);
  m_treeTrk->Branch("trk_corr_uv", &m_track.corrUV);
  m_treeTrk->Branch("trk_col", &m_track.col);
  m_treeTrk->Branch("trk_row", &m_track.row);
  m_treeTrk->Branch("trk_chi2", &m_track.chi2);
  m_treeTrk->Branch("trk_dof", &m_track.dof);
  m_treeTrk->Branch("trk_nclusters", &m_track.nClusters);
  m_treeTrk->Branch("mat_d2", &m_match.d2);
  setupCluster(m_treeTrk, m_clusterMatched);
  m_treeClu = new TTree("clusters_unmatched", "");
  m_treeClu->SetDirectory(sub);
  setupCluster(m_treeClu, m_clusterUnmatched);

  TTree* maskTree = new TTree("masked_pixels", "");
  int16_t maskedCol, maskedRow;
  maskTree->SetDirectory(sub);
  maskTree->Branch("col", &maskedCol);
  maskTree->Branch("row", &maskedRow);
  auto mask = m_sensor.pixelMask();
  for (Index c = 0; c < m_sensor.numCols(); ++c) {
    for (Index r = 0; r < m_sensor.numRows(); ++r) {
      if (mask.isMasked(c, r)) {
        maskedCol = static_cast<int16_t>(c);
        maskedRow = static_cast<int16_t>(r);
        maskTree->Fill();
      }
    }
  }
}

std::string Analyzers::MatchExporter::name() const { return m_name; }

void Analyzers::MatchExporter::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  auto fillCluster = [](const Storage::Cluster& cluster, ClusterData& data) {
    data.u = cluster.posLocal().x();
    data.v = cluster.posLocal().y();
    data.stdU = std::sqrt(cluster.covLocal()(0, 0));
    data.stdV = std::sqrt(cluster.covLocal()(1, 1));
    data.corrUV = cluster.covLocal()(0, 1) / (data.stdU * data.stdV);
    data.col = cluster.posPixel().x();
    data.row = cluster.posPixel().y();
    data.time = cluster.time();
    data.value = cluster.value();
    data.region = ((cluster.region() == kInvalidIndex) ? -1 : cluster.region());
    data.size = std::min(cluster.size(), int(MAX_CLUSTER_SIZE));
    data.sizeCol = cluster.sizeCol();
    data.sizeRow = cluster.sizeRow();
    for (int ihit = 0; ihit < data.size; ++ihit) {
      const Storage::Hit& hit = *cluster.getHit(ihit);
      data.hitCol[ihit] = hit.col();
      data.hitRow[ihit] = hit.row();
      data.hitTime[ihit] = hit.time();
      data.hitValue[ihit] = hit.value();
    }
  };
  auto invalidateCluster = [](ClusterData& data) {
    data.u = std::numeric_limits<float>::quiet_NaN();
    data.v = std::numeric_limits<float>::quiet_NaN();
    data.stdU = std::numeric_limits<float>::quiet_NaN();
    data.stdV = std::numeric_limits<float>::quiet_NaN();
    data.corrUV = std::numeric_limits<float>::quiet_NaN();
    data.col = -1;
    data.row = -1;
    data.time = -1;
    data.value = -1;
    data.region = -1;
    data.size = 0; // required to have empty hit information
    data.sizeCol = 0;
    data.sizeRow = 0;
  };

  // global event information
  m_event.timestamp = event.timestamp();
  m_event.nClusters = plane.numClusters();
  m_event.nTracks = plane.numStates();

  // export tracks and possible matched clusters
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    const Storage::TrackState& state = plane.getState(istate);
    const Storage::Track& track = *state.track();

    // track data
    XYPoint cr = m_sensor.transformLocalToPixel(state.offset());
    m_track.u = state.offset().x();
    m_track.v = state.offset().y();
    m_track.du = state.slope().x();
    m_track.dv = state.slope().y();
    m_track.stdU = std::sqrt(state.covOffset()(0, 0));
    m_track.stdV = std::sqrt(state.covOffset()(1, 1));
    m_track.corrUV = state.covOffset()(0, 1) / (m_track.stdU * m_track.stdV);
    m_track.col = cr.x();
    m_track.row = cr.y();
    m_track.chi2 = track.chi2();
    m_track.dof = track.degreesOfFreedom();
    m_track.nClusters = track.numClusters();

    // matching cluster data
    if (state.matchedCluster()) {
      const Storage::Cluster& cluster = *state.matchedCluster();
      // fill matching information
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      XYVector delta = cluster.posLocal() - state.offset();
      m_match.d2 = mahalanobisSquared(cov, delta);
      // fill cluster information
      fillCluster(cluster, m_clusterMatched);
    } else {
      // fill invalid data if no matching cluster exists
      m_match.d2 = std::numeric_limits<float>::quiet_NaN();
      invalidateCluster(m_clusterMatched);
    }
    m_treeTrk->Fill();
  }

  // export unmatched clusters
  for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *plane.getCluster(icluster);
    // already exported during track iteration
    if (cluster.matchedTrack())
      continue;
    fillCluster(cluster, m_clusterUnmatched);
    m_treeClu->Fill();
  }
}

void Analyzers::MatchExporter::finalize() {}
