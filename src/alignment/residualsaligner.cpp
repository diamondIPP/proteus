#include "residualsaligner.h"

#include <TDirectory.h>
#include <TH2.h>

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(ResidualsAligner)

Alignment::ResidualsAligner::ResidualsAligner(
    const Mechanics::Device& device,
    const std::vector<Index>& alignIds,
    TDirectory* dir,
    double damping)
    : m_device(device), m_damping(damping)
{
  TDirectory* sub = Utils::makeDir(dir, "ResidualsAligner");

  double maxSlope = 0.001;
  size_t numBins = 100;

  for (auto id = alignIds.begin(); id != alignIds.end(); ++id) {
    const Mechanics::Sensor& sensor = *device.getSensor(*id);
    double pixelScale = 2 * std::max(sensor.pitchCol(), sensor.pitchRow());

    auto makeH1 = [&](const char* name, double range) -> TH1D* {
      TH1D* h = new TH1D((sensor.name() + "-" + name).c_str(), "", numBins,
                         -range, range);
      h->SetDirectory(sub);
      return h;
    };

    Histograms hists;
    hists.sensorId = *id;
    hists.deltaU = makeH1("DeltaU", pixelScale);
    hists.deltaV = makeH1("DeltaV", pixelScale);
    m_hists.push_back(hists);
  }

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
  for (auto hists = m_hists.begin(); hists != m_hists.end(); ++hists) {
    Index sensorId = hists->sensorId;
    const Mechanics::Sensor& sensor = *m_device.getSensor(sensorId);
    const Storage::Plane& sensorEvent = *event.getPlane(sensorId);

    for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
      const Storage::Cluster& cluster = *sensorEvent.getCluster(iclu);
      const Storage::Track* track = cluster.track();

      if (!track)
        continue;

      // refit track w/o selected sensor for unbiased residuals
      Storage::TrackState state =
          Processors::fitTrackLocalUnbiased(*track, sensor);
      XYPoint clu = sensor.transformPixelToLocal(cluster.posPixel());
      XYVector res = clu - state.offset();

      hists->deltaU->Fill(res.x());
      hists->deltaV->Fill(res.y());
    }
  }
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);
    const Storage::TrackState& global = track.globalState();

    m_trackSlope->Fill(global.slope().x(), global.slope().y());
  }
}

void Alignment::ResidualsAligner::finalize() {}

Mechanics::Alignment Alignment::ResidualsAligner::updatedGeometry() const
{
  Mechanics::Alignment geo = m_device.geometry();

  double slopeX = m_trackSlope->GetMean(1);
  double slopeY = m_trackSlope->GetMean(2);
  geo.setBeamSlope(slopeX, slopeY);

  INFO("mean track slope:");
  INFO(" slope x: ", slopeX, " +- ", m_trackSlope->GetStdDev(1));
  INFO(" slope y: ", slopeY, " +- ", m_trackSlope->GetStdDev(2));

  for (auto hists = m_hists.begin(); hists != m_hists.end(); ++hists) {
    const Mechanics::Sensor& sensor = *m_device.getSensor(hists->sensorId);

    Vector6 delta(-m_damping * hists->deltaU->GetMean(),
                  -m_damping * hists->deltaV->GetMean(), 0, 0, 0, 0);
    geo.correctLocal(sensor.id(), delta);

    INFO(sensor.name(), " alignment corrections:");
    INFO("  delta u:  ", delta[0]);
    INFO("  delta v:  ", delta[1]);
  }
  return geo;
}
