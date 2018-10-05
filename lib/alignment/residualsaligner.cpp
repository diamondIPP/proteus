#include "residualsaligner.h"

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>

#include "mechanics/device.h"
#include "storage/event.h"
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
    const int bins)
    : m_device(device), m_damping(damping)
{
  using namespace Utils;

  for (auto id : alignIds) {
    const Mechanics::Sensor& sensor = device.getSensor(id);
    double offsetRange =
        pixelRange * std::hypot(sensor.pitchCol(), sensor.pitchRow());

    TDirectory* sub =
        makeDir(dir, "sensors/" + sensor.name() + "/aligner_residuals");

    HistAxis axU(-offsetRange, offsetRange, bins, "Local offset u correction");
    HistAxis axV(-offsetRange, offsetRange, bins, "Local offset v correction");
    HistAxis axGamma(-gammaRange, gammaRange, bins,
                     "Local rotation #gamma correction");
    SensorHists hists;
    hists.sensorId = id;
    hists.corrU = makeH1(sub, "correction_u", axU);
    hists.corrV = makeH1(sub, "correction_v", axV);
    hists.corrGamma = makeH1(sub, "correction_gamma", axGamma);
    m_hists.push_back(hists);
  }
}

std::string Alignment::ResidualsAligner::name() const
{
  return "ResidualsAligner";
}

void Alignment::ResidualsAligner::execute(const Storage::Event& event)
{
  for (auto& hists : m_hists) {
    Index isensor = hists.sensorId;
    const Storage::SensorEvent& sensorEvent = event.getSensorEvent(isensor);

    for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
      const Storage::Cluster& cluster = sensorEvent.getCluster(iclu);

      if (!cluster.isInTrack())
        continue;

      const Storage::TrackState& state =
          sensorEvent.getLocalState(cluster.track());
      double u = state.offset()[0];
      double v = state.offset()[1];
      double ru = cluster.posLocal()[0] - u;
      double rv = cluster.posLocal()[1] - v;
      // if we have no measurement uncertainties, the measured residuals are
      // fully defined by the three alignment corrections du, dv, dgamma as
      //
      //     res_u = -du + dgamma * v
      //     res_v = -dv - dgamma * u
      //
      // this underdetermined system (2 equations, 3 variables) can be solved
      // using the pseudo-inverse of the corresponding matrix equation.
      // this directly yields values for the three alignment parameters as
      // a function of the residuals (res_u, res_v) and the estimated track
      // position (u, v)
      double f = 1 + u * u + v * v;
      double du = -(ru + ru * u * u + rv * u * v) / f;
      double dv = -(rv + rv * v * v + ru * u * v) / f;
      double dgamma = (ru * v - rv * u) / f;

      hists.corrU->Fill(du);
      hists.corrV->Fill(dv);
      hists.corrGamma->Fill(dgamma);
    }
  }
}

Mechanics::Geometry Alignment::ResidualsAligner::updatedGeometry() const
{
  Mechanics::Geometry geo = m_device.geometry();

  for (const auto& hists : m_hists) {
    const Mechanics::Sensor& sensor = m_device.getSensor(hists.sensorId);
    const Mechanics::Plane& plane = geo.getPlane(hists.sensorId);
    constexpr int binOffset = -1;

    auto resDU = Utils::getRestrictedMean(hists.corrU, binOffset);
    double du = resDU.first;
    double varDU = resDU.second;

    auto resDV = Utils::getRestrictedMean(hists.corrV, binOffset);
    double dv = resDV.first;
    double varDV = resDV.second;

    auto resDGamma = Utils::getRestrictedMean(hists.corrGamma, binOffset);
    double dgamma = resDGamma.first;
    double varDGamma = resDGamma.second;

    // enforce vanishing dz
    Vector3 offsetGlobal = plane.rotation * Vector3(du, dv, 0.0);
    offsetGlobal[2] = 0.0;
    Vector3 offsetLocal = Transpose(plane.rotation) * offsetGlobal;

    // combined local corrections
    Vector6 delta;
    delta[0] = m_damping * offsetLocal[0];
    delta[1] = m_damping * offsetLocal[1];
    delta[2] = m_damping * offsetLocal[2];
    delta[3] = 0.0;
    delta[4] = 0.0;
    delta[5] = m_damping * dgamma;
    SymMatrix6 cov;
    cov(0, 0) = varDU;
    cov(1, 1) = varDV;
    cov(5, 5) = varDGamma;
    geo.correctLocal(hists.sensorId, delta, cov);

    // output w/ angles in degrees
    INFO(sensor.name(), " alignment corrections:");
    INFO("  du: ", delta[0], " ± ", std::sqrt(varDU));
    INFO("  dv: ", delta[1], " ± ", std::sqrt(varDV));
    INFO("  dw: ", delta[2], " (dz=0 enforced)");
    INFO("  dgamma: ", degree(delta[5]), " ± ", degree(std::sqrt(varDGamma)),
         " degree");
  }
  return geo;
}
