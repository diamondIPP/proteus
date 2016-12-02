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

  double gammaScale = 0.1;
  double slopeScale = 0.01;
  size_t numBins = 100;

  // per-sensor histograms
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
    hists.deltaGamma = makeH1("DeltaGamma", gammaScale);
    m_hists.push_back(hists);
  }

  // global track direction histograms
  XYZVector beamDir = device.geometry().beamDirection();
  double slopeX = beamDir.x() / beamDir.z();
  double slopeY = beamDir.y() / beamDir.z();
  m_trackSlope = new TH2D("TrackSlope", "", numBins, slopeX - slopeScale,
                          slopeX + slopeScale, numBins, slopeY - slopeScale,
                          slopeY + slopeScale);
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

      double u = state.offset().x();
      double v = state.offset().y();
      double ru = u - cluster.posLocal().x();
      double rv = v - cluster.posLocal().y();
      // if we have no measurement uncertainties, the measured residuals are
      // fully defined by the three alignment corrections du, dv, dgamma as
      //
      //     res_u = du - dgamma * v
      //     res_v = dv + dgamma * u
      //
      // this underdetermined system (2 equations, 3 variables) can be solved
      // using the pseudo-inverse of the corresponding matrix equation.
      // this directly yields values for the three alignment parameters as
      // a function of the residuals (res_u, res_v) and the estimated track
      // position (u, v)
      double f = 1 + u * u + v * v;
      double du = (ru + ru * u * u + rv * u * v) / f;
      double dv = (rv + rv * v * v + ru * u * v) / f;
      double dgamma = (rv * u - ru * v) / f;

      hists->deltaU->Fill(du);
      hists->deltaV->Fill(dv);
      hists->deltaGamma->Fill(dgamma);
    }
  }
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);
    const Storage::TrackState& global = track.globalState();

    m_trackSlope->Fill(global.slope().x(), global.slope().y());
  }
}

void Alignment::ResidualsAligner::finalize() {}

Mechanics::Geometry Alignment::ResidualsAligner::updatedGeometry() const
{
  Mechanics::Geometry geo = m_device.geometry();

  double slopeX = m_trackSlope->GetMean(1);
  double slopeY = m_trackSlope->GetMean(2);
  geo.setBeamSlope(slopeX, slopeY);

  INFO("mean track slope:");
  INFO(" slope x: ", slopeX, " +- ", m_trackSlope->GetStdDev(1));
  INFO(" slope y: ", slopeY, " +- ", m_trackSlope->GetStdDev(2));

  for (auto hists = m_hists.begin(); hists != m_hists.end(); ++hists) {
    const Mechanics::Sensor& sensor = *m_device.getSensor(hists->sensorId);

    Vector6 delta;
    delta[0] = m_damping * hists->deltaU->GetMean();
    delta[1] = m_damping * hists->deltaV->GetMean();
    delta[5] = m_damping * hists->deltaGamma->GetMean();
    double stdU = hists->deltaU->GetMeanError();
    double stdV = hists->deltaU->GetMeanError();
    double stdGamma = hists->deltaGamma->GetMeanError();
    SymMatrix6 cov;
    cov(0, 0) = stdU * stdU;
    cov(1, 1) = stdV * stdV;
    cov(5, 5) = stdGamma * stdGamma;

    geo.correctLocal(sensor.id(), delta, cov);

    INFO(sensor.name(), " alignment corrections:");
    INFO("  delta u:  ", delta[0], " +- ", stdU);
    INFO("  delta v:  ", delta[1], " +- ", stdV);
    INFO("  delta gamma:", delta[5], " +- ", stdGamma);
  }
  return geo;
}
