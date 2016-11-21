#include "residualsaligner.h"

#include <TDirectory.h>
#include <TH2.h>

#include "analyzers/residuals.h"
#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(ResidualsAligner)

Alignment::ResidualsAligner::ResidualsAligner(
    const Mechanics::Device& device,
    const std::vector<Index>& alignIds,
    TDirectory* dir)
    : m_device(device), m_alignIds(alignIds), m_res(device, dir)
{
  TDirectory* sub = Utils::makeDir(dir, "ResidualsAligner");

  double maxSlope = 0.001;
  size_t numBins = 100;
  m_trackSlope = new TH2D("TrackSlope", "", numBins, -maxSlope, maxSlope,
                          numBins, -maxSlope, maxSlope);
  m_trackSlope->SetDirectory(sub);
}

std::string Alignment::ResidualsAligner::name() const
{
  return "ResidualsAligner";
}

void Alignment::ResidualsAligner::analyze(const Storage::Event& event)
{
  m_res.analyze(event);
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);
    const Storage::TrackState& global = track.globalState();

    m_trackSlope->Fill(global.slope().x(), global.slope().y());
  }
}

void Alignment::ResidualsAligner::finalize() { m_res.finalize(); }

Mechanics::Alignment Alignment::ResidualsAligner::updatedGeometry() const
{
  Mechanics::Alignment geo = m_device.alignment();

  double slopeX = m_trackSlope->GetMean(1);
  double slopeY = m_trackSlope->GetMean(2);
  geo.setBeamSlope(slopeX, slopeY);

  INFO("mean track slope:");
  INFO(" slope x: ", slopeX, " +- ", m_trackSlope->GetStdDev(1));
  INFO(" slope y: ", slopeY, " +- ", m_trackSlope->GetStdDev(2));

  for (auto id = m_alignIds.begin(); id != m_alignIds.end(); ++id) {
    const TH2D* resUV = m_res.getResidualUV(*id);

    Vector3 dq(-resUV->GetMean(1), -resUV->GetMean(2), 0);
    Matrix3 dr = ROOT::Math::SMatrixIdentity();
    geo.correctLocal(*id, dq, dr);

    INFO(m_device.getSensor(*id)->name(), *id, " alignment corrections:");
    INFO("  delta u:  ", dq[0]);
    INFO("  delta v:  ", dq[1]);
  }
  return geo;
}
