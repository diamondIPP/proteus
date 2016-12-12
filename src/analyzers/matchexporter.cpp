#include "matchexporter.h"

#include <limits>
#include <set>
#include <string>

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

  m_treeTrk = new TTree("tracks", "");
  m_treeTrk->SetDirectory(sub);
  m_treeTrk->Branch("ev_ntracks", &m_ev.nTracks, "ev_ntracks/I");
  m_treeTrk->Branch("trk_redchi2", &m_trk.redChi2, "trk_redchi2/F");
  m_treeTrk->Branch("trk_u", &m_trk.u, "trk_u/F");
  m_treeTrk->Branch("trk_v", &m_trk.v, "trk_v/F");
  m_treeTrk->Branch("trk_col", &m_trk.col, "trk_col/F");
  m_treeTrk->Branch("trk_row", &m_trk.row, "trk_row/F");
  m_treeTrk->Branch("d2", &m_mat.d2, "d2/F");
  m_treeTrk->Branch("clu_u", &m_cluMat.u, "clu_u/F");
  m_treeTrk->Branch("clu_v", &m_cluMat.v, "clu_v/F");
  m_treeTrk->Branch("clu_col", &m_cluMat.col, "clu_col/F");
  m_treeTrk->Branch("clu_row", &m_cluMat.row, "clu_row/F");
  m_treeTrk->Branch("clu_size", &m_cluMat.size, "clu_size/I");
  m_treeTrk->Branch("clu_size_col", &m_cluMat.sizeCol, "clu_size_col/I");
  m_treeTrk->Branch("clu_size_row", &m_cluMat.sizeRow, "clu_size_row/I");

  m_treeClu = new TTree("clusters_unmatched", "");
  m_treeClu->SetDirectory(sub);
  m_treeClu->Branch("u", &m_cluUnm.u, "u/F");
  m_treeClu->Branch("v", &m_cluUnm.v, "v/F");
  m_treeClu->Branch("col", &m_cluUnm.col, "col/F");
  m_treeClu->Branch("row", &m_cluUnm.row, "row/F");
  m_treeClu->Branch("size", &m_cluUnm.size, "size/I");
  m_treeClu->Branch("sizeCol", &m_cluUnm.sizeCol, "sizeCol/I");
  m_treeClu->Branch("sizeRow", &m_cluUnm.sizeRow, "sizeRow/I");
}

std::string Analyzers::MatchExporter::name() const { return m_name; }

void Analyzers::MatchExporter::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);
  Index numMatches = 0;

  // global event information
  m_ev.nTracks = event.numTracks();

  // export tracks and possible matched clusters
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    const Storage::TrackState& state = plane.getState(istate);

    // track data
    XYPoint cr = m_sensor.transformLocalToPixel(state.offset());
    m_trk.redChi2 = state.track()->reducedChi2();
    m_trk.u = state.offset().x();
    m_trk.v = state.offset().y();
    m_trk.col = cr.x();
    m_trk.row = cr.y();

    // matching cluster data
    if (state.matchedCluster()) {
      const Storage::Cluster& cluster = *state.matchedCluster();
      XYVector delta = cluster.posLocal() - state.offset();
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      m_mat.d2 = mahalanobisSquared(cov, delta);
      m_cluMat.u = cluster.posLocal().x();
      m_cluMat.v = cluster.posLocal().y();
      m_cluMat.col = cluster.posPixel().x();
      m_cluMat.row = cluster.posPixel().y();
      m_cluMat.size = cluster.size();
      m_cluMat.sizeCol = cluster.sizeCol();
      m_cluMat.sizeRow = cluster.sizeRow();
      numMatches += 1;
    } else {
      m_mat.d2 = -1;
      m_cluMat.u = std::numeric_limits<Float_t>::quiet_NaN();
      m_cluMat.v = std::numeric_limits<Float_t>::quiet_NaN();
      m_cluMat.col = -1;
      m_cluMat.row = -1;
      m_cluMat.size = -1;
      m_cluMat.sizeCol = -1;
      m_cluMat.sizeRow = -1;
    }
    m_treeTrk->Fill();
  }

  // export unmatched clusters
  for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *plane.getCluster(icluster);
    // already exported during track iteration
    if (cluster.matchedTrack())
      continue;

    XYPoint uv = m_sensor.transformPixelToLocal(cluster.posPixel());
    m_cluUnm.u = uv.x();
    m_cluUnm.v = uv.y();
    m_cluUnm.col = cluster.posPixel().x();
    m_cluUnm.row = cluster.posPixel().y();
    m_cluUnm.size = cluster.size();
    m_cluUnm.sizeCol = cluster.sizeCol();
    m_cluUnm.sizeRow = cluster.sizeRow();
    m_treeClu->Fill();
  }

  m_statMat.fill(numMatches);
  m_statUnmTrk.fill(event.numTracks() - numMatches);
  m_statUnmClu.fill(plane.numClusters() - numMatches);
}

void Analyzers::MatchExporter::finalize()
{
  INFO("matching for ", m_sensor.name(), ':');
  INFO("  matched/event: ", m_statMat);
  INFO("  unmatched tracks/event: ", m_statUnmTrk);
  INFO("  unmatched clusters/event: ", m_statUnmClu);
}
