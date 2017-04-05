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

  auto setupCluster = [](TTree* tree, const std::string& prefix,
                         ClusterData& data) {
    tree->Branch((prefix + "u").c_str(), &data.u);
    tree->Branch((prefix + "v").c_str(), &data.v);
    tree->Branch((prefix + "std_u").c_str(), &data.stdU);
    tree->Branch((prefix + "std_v").c_str(), &data.stdV);
    tree->Branch((prefix + "corr_uv").c_str(), &data.corrUV);
    tree->Branch((prefix + "col").c_str(), &data.col);
    tree->Branch((prefix + "row").c_str(), &data.row);
    tree->Branch((prefix + "time").c_str(), &data.time);
    tree->Branch((prefix + "value").c_str(), &data.value);
    tree->Branch((prefix + "region").c_str(), &data.region);
    tree->Branch((prefix + "size").c_str(), &data.size);
    tree->Branch((prefix + "size_col").c_str(), &data.sizeCol);
    tree->Branch((prefix + "size_row").c_str(), &data.sizeRow);
    tree->Branch((prefix + "hit_col").c_str(), &data.hitCol,
                 (prefix + "hit_col[" + prefix + "size]/S").c_str());
    tree->Branch((prefix + "hit_row").c_str(), &data.hitRow,
                 (prefix + "hit_row[" + prefix + "size]/S").c_str());
    tree->Branch((prefix + "hit_time").c_str(), &data.hitTime,
                 (prefix + "hit_time[" + prefix + "size]/F").c_str());
    tree->Branch((prefix + "hit_value").c_str(), &data.hitValue,
                 (prefix + "hit_value[" + prefix + "size]/F").c_str());
  };

  m_treeTrk = new TTree("tracks", "");
  m_treeTrk->SetDirectory(sub);
  m_treeTrk->Branch("evt_ntracks", &m_event.nTracks);
  m_treeTrk->Branch("trk_u", &m_track.u);
  m_treeTrk->Branch("trk_v", &m_track.v);
  m_treeTrk->Branch("trk_std_u", &m_track.stdU);
  m_treeTrk->Branch("trk_std_v", &m_track.stdV);
  m_treeTrk->Branch("trk_corr_uv", &m_track.corrUV);
  m_treeTrk->Branch("trk_col", &m_track.col);
  m_treeTrk->Branch("trk_row", &m_track.row);
  m_treeTrk->Branch("trk_chi2", &m_track.chi2);
  m_treeTrk->Branch("trk_dof", &m_track.dof);
  m_treeTrk->Branch("trk_nclusters", &m_track.nClusters);
  m_treeTrk->Branch("mat_d2", &m_match.d2);
  setupCluster(m_treeTrk, "clu_", m_clusterMatched);
  m_treeClu = new TTree("clusters_unmatched", "");
  m_treeClu->SetDirectory(sub);
  setupCluster(m_treeClu, std::string(), m_clusterUnmatched);
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
  m_event.nTracks = event.numTracks();
  // export tracks and possible matched clusters
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    const Storage::TrackState& state = plane.getState(istate);
    const Storage::Track& track = *state.track();

    // track data
    XYPoint cr = m_sensor.transformLocalToPixel(state.offset());
    m_track.u = state.offset().x();
    m_track.v = state.offset().y();
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
