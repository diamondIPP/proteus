// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "residualsaligner.h"

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

namespace proteus {

ResidualsAligner::ResidualsAligner(TDirectory* dir,
                                   const Device& device,
                                   const std::vector<Index>& alignIds,
                                   const double damping,
                                   const double pixelRange,
                                   const double gammaRange,
                                   const int bins)
    : m_device(device), m_damping(damping)
{

  for (auto id : alignIds) {
    const Sensor& sensor = device.getSensor(id);
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

std::string ResidualsAligner::name() const { return "ResidualsAligner"; }

void ResidualsAligner::execute(const Event& event)
{
  for (auto& hists : m_hists) {
    Index isensor = hists.sensorId;
    const SensorEvent& sensorEvent = event.getSensorEvent(isensor);

    for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
      const Cluster& cluster = sensorEvent.getCluster(iclu);

      if (!cluster.isInTrack())
        continue;

      const TrackState& state = sensorEvent.getLocalState(cluster.track());
      double u = state.loc0();
      double v = state.loc1();
      double ru = cluster.u() - u;
      double rv = cluster.v() - v;
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

Geometry ResidualsAligner::updatedGeometry() const
{
  // how many bins are used to calculated the means
  static constexpr int kBinsRestricted = 5;

  Geometry geo = m_device.geometry();

  for (const auto& hists : m_hists) {
    const Sensor& sensor = m_device.getSensor(hists.sensorId);
    const Plane& plane = geo.getPlane(hists.sensorId);

    auto resDU = getRestrictedMean(hists.corrU, kBinsRestricted);
    auto resDV = getRestrictedMean(hists.corrV, kBinsRestricted);
    auto resDGamma = getRestrictedMean(hists.corrGamma, kBinsRestricted);
    auto du = resDU.first;
    auto varDU = resDU.second;
    auto dv = resDV.first;
    auto varDV = resDV.second;
    auto dgamma = resDGamma.first;
    auto varDGamma = resDGamma.second;

    // enforce vanishing dz
    Vector4 offsetLocal = Vector4::Zero();
    offsetLocal[kU] = du;
    offsetLocal[kV] = dv;
    Vector4 offsetGlobal = plane.linearToGlobal() * offsetLocal;
    offsetGlobal[kZ] = 0.0;
    offsetLocal = plane.linearToLocal() * offsetGlobal;

    // combined local corrections
    Vector6 delta = Vector6::Zero();
    delta[0] = m_damping * offsetLocal[kU];
    delta[1] = m_damping * offsetLocal[kV];
    delta[2] = m_damping * offsetLocal[kW];
    delta[5] = m_damping * dgamma;
    SymMatrix6 cov = SymMatrix6::Zero();
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

} // namespace proteus
