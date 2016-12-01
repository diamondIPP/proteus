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
    , m_name("MatchExporter_" + device.getSensor(sensorId)->name())
{
  TDirectory* sub = dir->mkdir(m_sensor.name().c_str());

  m_ttrack.tree = new TTree("tracks", "");
  m_ttrack.tree->SetDirectory(sub);
  m_ttrack.tree->Branch("trk_redchi2", &m_ttrack.trkRedChi2, "trk_redchi2/F");
  m_ttrack.tree->Branch("trk_u", &m_ttrack.trkU, "trk_u/F");
  m_ttrack.tree->Branch("trk_v", &m_ttrack.trkV, "trk_v/F");
  m_ttrack.tree->Branch("dist", &m_ttrack.dist, "dist/F");
  m_ttrack.tree->Branch("clu_u", &m_ttrack.cluU, "clu_u/F");
  m_ttrack.tree->Branch("clu_v", &m_ttrack.cluU, "clu_v/F");
  m_ttrack.tree->Branch("clu_col", &m_ttrack.cluU, "clu_col/F");
  m_ttrack.tree->Branch("clu_row", &m_ttrack.cluV, "clu_row/F");
  m_ttrack.tree->Branch("clu_size", &m_ttrack.cluSize, "clu_size/I");
  m_ttrack.tree->Branch("clu_size_col", &m_ttrack.cluSizeCol, "clu_size_col/I");
  m_ttrack.tree->Branch("clu_size_row", &m_ttrack.cluSizeRow, "clu_size_row/I");

  m_tcluster.tree = new TTree("clusters_unmatched", "");
  m_tcluster.tree->SetDirectory(sub);
  m_tcluster.tree->Branch("u", &m_tcluster.u, "u/F");
  m_tcluster.tree->Branch("v", &m_tcluster.v, "v/F");
  m_tcluster.tree->Branch("col", &m_tcluster.col, "col/F");
  m_tcluster.tree->Branch("row", &m_tcluster.row, "row/F");
  m_tcluster.tree->Branch("size", &m_tcluster.size, "size/I");
  m_tcluster.tree->Branch("sizeCol", &m_tcluster.sizeCol, "sizeCol/I");
  m_tcluster.tree->Branch("sizeRow", &m_tcluster.sizeRow, "sizeRow/I");
}

std::string Analyzers::MatchExporter::name() const { return m_name; }

void Analyzers::MatchExporter::analyze(const Storage::Event& event)
{
  Index numMatches = 0;

  // export tracks and possible matched clusters
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);

    if (!track.hasLocalState(m_sensorId))
      continue;

    m_ttrack.trkRedChi2 = track.reducedChi2();
    const Storage::TrackState& state = track.getLocalState(m_sensorId);
    m_ttrack.trkU = state.offset().x();
    m_ttrack.trkV = state.offset().y();

    if (track.hasMatchedCluster(m_sensorId)) {
      const Storage::Cluster& cluster = *track.getMatchedCluster(m_sensorId);
      XYVector delta = cluster.posLocal() - state.offset();
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      m_ttrack.dist = mahalanobisSquared(cov, delta);
      m_ttrack.cluU = cluster.posLocal().x();
      m_ttrack.cluV = cluster.posLocal().y();
      m_ttrack.cluCol = cluster.posPixel().x();
      m_ttrack.cluRow = cluster.posPixel().y();
      m_ttrack.cluSize = cluster.size();
      m_ttrack.cluSizeCol = cluster.sizeCol();
      m_ttrack.cluSizeRow = cluster.sizeRow();
      numMatches += 1;
    } else {
      m_ttrack.dist = std::numeric_limits<Float_t>::quiet_NaN();
      m_ttrack.cluU = std::numeric_limits<Float_t>::quiet_NaN();
      m_ttrack.cluV = std::numeric_limits<Float_t>::quiet_NaN();
      m_ttrack.cluCol = -1;
      m_ttrack.cluRow = -1;
      m_ttrack.cluSize = -1;
      m_ttrack.cluSizeCol = -1;
      m_ttrack.cluSizeRow = -1;
    }
    m_ttrack.tree->Fill();
  }

  // export unmatched clusters
  const Storage::Plane& plane = *event.getPlane(m_sensorId);
  for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *plane.getCluster(icluster);
    // already exported during track iteration
    if (cluster.hasMatchedTrack())
      continue;

    XYPoint uv = m_sensor.transformPixelToLocal(cluster.posPixel());
    m_tcluster.u = uv.x();
    m_tcluster.v = uv.y();
    m_tcluster.col = cluster.posPixel().x();
    m_tcluster.row = cluster.posPixel().y();
    m_tcluster.size = cluster.size();
    m_tcluster.sizeCol = cluster.sizeCol();
    m_tcluster.sizeRow = cluster.sizeRow();
    m_tcluster.tree->Fill();
  }

  m_matched.fill(numMatches);
  m_unmatchedTracks.fill(event.numTracks() - numMatches);
  m_unmachedClusters.fill(plane.numClusters() - numMatches);
}

void Analyzers::MatchExporter::finalize()
{
  INFO("matching for ", m_sensor.name(), ':');
  INFO("  matched/event: ", m_matched);
  INFO("  unmatched tracks/event: ", m_unmatchedTracks);
  INFO("  unmatched clusters/event: ", m_unmachedClusters);
}
