#include "residualsaligner.h"

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>

#include "analyzers/tracks.h"
#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/tracking.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(ResidualsAligner)

Alignment::ResidualsAligner::ResidualsAligner(
    TDirectory* dir,
    const Mechanics::Device& device,
    const std::vector<Index>& alignIds,
    const double damping,
    const double pixelRange,
    const double gammaRange,
    const double slopeRange,
    const int bins)
    : m_tracks(new Analyzers::Tracks(dir, device))
    , m_device(device)
    , m_damping(damping)
{
  using namespace Utils;

  for (auto id = alignIds.begin(); id != alignIds.end(); ++id) {
    const Mechanics::Sensor& sensor = *device.getSensor(*id);
    double offsetRange =
        pixelRange * std::hypot(sensor.pitchCol(), sensor.pitchRow());

    TDirectory* sub = makeDir(dir, sensor.name() + "/aligner_residuals");

    HistAxis axU(-offsetRange, offsetRange, bins, "Local offset u correction");
    HistAxis axV(-offsetRange, offsetRange, bins, "Local offset v correction");
    HistAxis axGamma(-gammaRange, gammaRange, bins,
                     "Local rotation #gamma correction");
    SensorHists hists;
    hists.sensorId = *id;
    hists.corrU = makeH1(sub, "correction_u", axU);
    hists.corrV = makeH1(sub, "correction_v", axV);
    hists.corrGamma = makeH1(sub, "correction_gamma", axGamma);
    m_hists.push_back(hists);
  }
}

// required to make pImpled unique_ptr work
Alignment::ResidualsAligner::~ResidualsAligner() {}

std::string Alignment::ResidualsAligner::name() const
{
  return "ResidualsAligner";
}

void Alignment::ResidualsAligner::analyze(const Storage::Event& event)
{
  for (const auto& hists : m_hists) {
    Index sensorId = hists.sensorId;
    const Storage::SensorEvent& sensorEvent = event.getSensorEvent(sensorId);

    for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
      const Storage::Cluster& cluster = *sensorEvent.getCluster(iclu);

      if (!cluster.isInTrack())
        continue;

      // refit track w/o selected sensor for unbiased residuals
      const Storage::Track& track = event.getTrack(cluster.track());
      Storage::TrackState state = Processors::fitTrackLocalUnbiased(
          track, m_device.geometry(), sensorId);

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

      hists.corrU->Fill(du);
      hists.corrV->Fill(dv);
      hists.corrGamma->Fill(dgamma);
    }
  }
  m_tracks->analyze(event);
}

void Alignment::ResidualsAligner::finalize() { m_tracks->finalize(); }

Mechanics::Geometry Alignment::ResidualsAligner::updatedGeometry() const
{
  Mechanics::Geometry geo = m_device.geometry();

  double slopeX = m_tracks->histSlopeX()->GetMean();
  double slopeXStd = m_tracks->histSlopeX()->GetStdDev();
  double slopeY = m_tracks->histSlopeY()->GetMean();
  double slopeYStd = m_tracks->histSlopeY()->GetStdDev();
  geo.setBeamSlope(slopeX, slopeY);

  INFO("mean track slope:");
  INFO("  slope x: ", slopeX, " +- ", slopeXStd);
  INFO("  slope y: ", slopeY, " +- ", slopeYStd);

  for (auto hists = m_hists.begin(); hists != m_hists.end(); ++hists) {
    const Mechanics::Sensor& sensor = *m_device.getSensor(hists->sensorId);

    Vector6 delta;
    delta[0] = m_damping * hists->corrU->GetMean();
    delta[1] = m_damping * hists->corrV->GetMean();
    delta[5] = m_damping * hists->corrGamma->GetMean();
    double stdU = hists->corrU->GetMeanError();
    double stdV = hists->corrU->GetMeanError();
    double stdGamma = hists->corrGamma->GetMeanError();
    SymMatrix6 cov;
    cov(0, 0) = stdU * stdU;
    cov(1, 1) = stdV * stdV;
    cov(5, 5) = stdGamma * stdGamma;

    geo.correctLocal(sensor.id(), delta, cov);

    INFO(sensor.name(), " alignment corrections:");
    INFO("  u: ", delta[0], " +- ", stdU);
    INFO("  v: ", delta[1], " +- ", stdV);
    INFO("  gamma: ", delta[5], " +- ", stdGamma);
  }
  return geo;
}
