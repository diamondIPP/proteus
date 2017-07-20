#include "match.h"

#include <cassert>
#include <limits>
#include <string>

#include <TDirectory.h>
#include <TTree.h>

#include "mechanics/device.h"
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

void Io::MatchWriter::EventData::set(const Storage::Event& e,
                                     const Storage::Plane& s)
{
  frame = e.frame();
  timestamp = e.timestamp();
  nClusters = s.numClusters();
  nTracks = e.numTracks();
}

void Io::MatchWriter::TrackData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("trk_u", &u);
  tree->Branch("trk_v", &v);
  tree->Branch("trk_du", &du);
  tree->Branch("trk_dv", &dv);
  tree->Branch("trk_std_u", &stdU);
  tree->Branch("trk_std_v", &stdV);
  tree->Branch("trk_corr_uv", &corrUV);
  tree->Branch("trk_col", &col);
  tree->Branch("trk_row", &row);
  tree->Branch("trk_chi2", &chi2);
  tree->Branch("trk_dof", &dof);
  tree->Branch("trk_size", &size);
}

void Io::MatchWriter::ClusterData::addToTree(TTree* tree)
{
  assert(tree);

  tree->Branch("clu_u", &u);
  tree->Branch("clu_v", &v);
  tree->Branch("clu_std_u", &stdU);
  tree->Branch("clu_std_v", &stdV);
  tree->Branch("clu_corr_uv", &corrUV);
  tree->Branch("clu_col", &col);
  tree->Branch("clu_row", &row);
  tree->Branch("clu_time", &time);
  tree->Branch("clu_value", &value);
  tree->Branch("clu_region", &region);
  tree->Branch("clu_size", &size);
  tree->Branch("clu_size_col", &sizeCol);
  tree->Branch("clu_size_row", &sizeRow);
  tree->Branch("hit_col", &hitCol, "hit_col[clu_size]/S");
  tree->Branch("hit_row", &hitRow, "hit_row[clu_size]/S");
  tree->Branch("hit_time", &hitTime, "hit_time[clu_size]/F");
  tree->Branch("hit_value", &hitValue, "hit_value[clu_size]/F");
}

void Io::MatchWriter::ClusterData::set(const Storage::Cluster& c)
{
  u = c.posLocal().x();
  v = c.posLocal().y();
  stdU = std::sqrt(c.covLocal()(0, 0));
  stdV = std::sqrt(c.covLocal()(1, 1));
  corrUV = c.covLocal()(0, 1) / (stdU * stdV);
  col = c.posPixel().x();
  row = c.posPixel().y();
  time = c.time();
  value = c.value();
  region = ((c.region() == kInvalidIndex) ? -1 : c.region());
  size = std::min(c.size(), int(MAX_CLUSTER_SIZE));
  sizeCol = c.sizeCol();
  sizeRow = c.sizeRow();
  for (int ihit = 0; ihit < size; ++ihit) {
    const Storage::Hit& hit = *c.getHit(ihit);
    hitCol[ihit] = hit.col();
    hitRow[ihit] = hit.row();
    hitTime[ihit] = hit.time();
    hitValue[ihit] = hit.value();
  }
}

void Io::MatchWriter::ClusterData::invalidate()
{
  u = std::numeric_limits<float>::quiet_NaN();
  v = std::numeric_limits<float>::quiet_NaN();
  stdU = std::numeric_limits<float>::quiet_NaN();
  stdV = std::numeric_limits<float>::quiet_NaN();
  corrUV = std::numeric_limits<float>::quiet_NaN();
  col = std::numeric_limits<float>::quiet_NaN();
  row = std::numeric_limits<float>::quiet_NaN();
  time = std::numeric_limits<float>::quiet_NaN();
  value = std::numeric_limits<float>::quiet_NaN();
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

Io::MatchWriter::MatchWriter(const Mechanics::Device& device,
                             Index sensorId,
                             TDirectory* dir)
    : m_sensor(*device.getSensor(sensorId))
    , m_sensorId(sensorId)
    , m_name("MatchWriter(" + device.getSensor(sensorId)->name() + ')')
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
  auto mask = m_sensor.pixelMask();
  for (Index c = 0; c < m_sensor.numCols(); ++c) {
    for (Index r = 0; r < m_sensor.numRows(); ++r) {
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
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  m_event.set(event, plane);

  // export tracks and possible matched clusters
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    const Storage::TrackState& state = plane.getState(istate);
    const Storage::Track& track = *state.track();

    // always set track data
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
    m_track.size = track.numClusters();

    // matching cluster data
    if (state.matchedCluster()) {
      const Storage::Cluster& cluster = *state.matchedCluster();
      // set cluster information
      m_matchedCluster.set(cluster);
      // set matching information
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      XYVector delta = cluster.posLocal() - state.offset();
      m_matchedDist.d2 = mahalanobisSquared(cov, delta);
    } else {
      // fill invalid data if no matching cluster exists
      m_matchedCluster.invalidate();
      m_matchedDist.d2 = std::numeric_limits<float>::quiet_NaN();
    }
    m_matchedTree->Fill();
  }

  // export unmatched clusters
  for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *plane.getCluster(icluster);

    // already exported during track iteration
    if (cluster.matchedTrack())
      continue;

    m_unmatchCluster.set(cluster);
    m_unmatchTree->Fill();
  }
}