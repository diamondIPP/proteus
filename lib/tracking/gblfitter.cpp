/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "gblfitter.h"

#include <cassert>
#include <cmath>

#include <GblPoint.h>
#include <GblTrajectory.h>

#include "mechanics/device.h"
#include "mechanics/geometry.h"
#include "mechanics/sensor.h"
#include "storage/event.h"
#include "tracking/propagation.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(GblFitter)

Tracking::GblFitter::GblFitter(const Mechanics::Device& device)
    : m_device(device)
    // sensor ids sorted along the expected propagation order
    , m_propagationIds(
          Mechanics::sortedAlongBeam(m_device.geometry(), m_device.sensorIds()))
{
}

std::string Tracking::GblFitter::name() const { return "GBLFitter"; }

namespace {

/// Mapping matrices between the proteus and the GBL parameter ordering
///
/// GBL expects parameter ordering [q/p, u', v', u, v].
struct Reorder {
  Matrix<Scalar, 5, 6> toGbl;
  Matrix<Scalar, 6, 5> toProteus;

  Reorder()
      : toGbl(Matrix<Scalar, 5, 6>::Zero())
      , toProteus(Matrix<Scalar, 6, 5>::Zero())
  {
    // map time slope to q/p to avoid singularities
    toGbl(0, kSlopeTime) = 1;
    toGbl(1, kSlopeLoc0) = 1;
    toGbl(2, kSlopeLoc1) = 1;
    toGbl(3, kLoc0) = 1;
    toGbl(4, kLoc1) = 1;
    toProteus(kLoc0, 3) = 1;
    toProteus(kLoc1, 4) = 1;
    toProteus(kSlopeLoc0, 1) = 1;
    toProteus(kSlopeLoc1, 2) = 1;
    toProteus(kSlopeTime, 0) = 1;
  }
};

} // namespace

void Tracking::GblFitter::execute(Storage::Event& event) const
{
  using gbl::GblPoint;
  using gbl::GblTrajectory;

  Reorder reorder;
  // temporary (resuable) storage
  Eigen::MatrixXd referenceParams(6, m_propagationIds.size());
  std::vector<GblPoint> gblPoints(m_propagationIds.size(), {Matrix5::Zero()});
  Eigen::VectorXd gblCorrection(5);
  Eigen::MatrixXd gblCovariance(5, 5);
  Eigen::VectorXd gblResiduals(2);
  Eigen::VectorXd gblErrorsMeasurements(2);
  Eigen::VectorXd gblErrorsResiduals(2);
  Eigen::VectorXd gblDownWeights(2);

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // Reference track in global coordinates
    Vector4 globalPos = track.globalState().position();
    Vector4 globalTan = track.globalState().tangent();

    // Propagate reference track through all sensor to define GBL trajectory
    for (size_t ipoint = 0; ipoint < m_propagationIds.size(); ++ipoint) {
      auto sensorId = m_propagationIds[ipoint];
      const auto& plane = m_device.geometry().getPlane(sensorId);

      // 1. Propagate track state to the plane intersection

      // equivalent state in local parameters
      Vector4 localPos = plane.toLocal(globalPos);
      Vector4 localTan = plane.linearToLocal() * globalTan;
      // distance of initial point to the intersection along the plane normal
      Scalar w0 = localPos[kW];
      // convert tangent to slope parametrization
      localTan /= localTan[kW];
      // propagate position to the intersection, tangent is invariant
      localPos -= w0 * localTan;

      // 2. Compute local track parameters to be used as reference later on

      referenceParams(kLoc0, ipoint) = localPos[kU];
      referenceParams(kLoc1, ipoint) = localPos[kV];
      referenceParams(kTime, ipoint) = localPos[kS];
      // tangent is already in slope parametrization
      referenceParams(kSlopeLoc0, ipoint) = localTan[kU];
      referenceParams(kSlopeLoc1, ipoint) = localTan[kV];
      referenceParams(kSlopeTime, ipoint) = localTan[kS];

      // 3. Compute Jacobian from previous to current plane

      Matrix6 jac;
      if (ipoint == 0) {
        // first point has no predecessor and no propagation Jacobian.
        jac = Matrix6::Identity();
      } else {
        auto prevSensorId = m_propagationIds[ipoint - 1];
        const auto& prev = m_device.geometry().getPlane(prevSensorId);
        Vector4 prevTan = prev.linearToLocal() * globalTan;
        Matrix4 prevToLocal = plane.linearToLocal() * prev.linearToGlobal();
        jac = Tracking::jacobianState(prevTan, prevToLocal, w0);
      }

      // 4. Create a GBL point for this step

      auto& point = gblPoints[ipoint];
      point = GblPoint(reorder.toGbl * jac * reorder.toProteus);

      // 4a. Add a scatterer for all inner points

      if ((0 < ipoint) and ((ipoint + 1) < m_propagationIds.size())) {
        const auto& sensor = m_device.getSensor(sensorId);
        // Define scattering in the local system w/ vanishing initial kink
        point.addScatterer(Vector2::Zero(), sensor.scatteringSlopePrecision());
      }

      // 4b. If available, add a measurement

      auto ic = track.clusters().find(sensorId);
      if (ic != track.clusters().end()) {
        const Storage::Cluster& cluster = ic->second;

        // Get the measurement (residuals)
        Vector2 meas(cluster.u() - localPos[kU], cluster.v() - localPos[kV]);
        // Get the measurement precision
        Matrix2 measPrec = cluster.uvCov().inverse();

        // Add measurement to the point, measurements and track parameters
        // are defined in the same coordinates and no projection is required.
        point.addMeasurement(meas, measPrec);
      }

      // 5. Update starting point for the next step

      globalPos = plane.toGlobal(localPos);
      // the tangent is a constant of the motion and remains unchanged
    }

    // fit the GBL trajectory w/o track curvature

    GblTrajectory traj(gblPoints, false);
    double chi2;
    double lostWeight;
    int dof;
    // TODO what to do for errors?
    auto ret = traj.fit(chi2, dof, lostWeight);
    track.setGoodnessOfFit(chi2, dof);
    DEBUG("fit ret: ", ret);

    // extract fitted local track states for all sensors

    for (size_t ipoint = 0; ipoint < m_propagationIds.size(); ++ipoint) {
      auto sensorId = m_propagationIds[ipoint];
      // GBL label starts counting at 1, w/ positive values indicating that
      // we want to get the parameters before the scatterer.
      auto label = ipoint + 1;

      traj.getResults(label, gblCorrection, gblCovariance);
      event.getSensorEvent(sensorId).setLocalState(
          itrack,
          referenceParams.col(ipoint) + reorder.toProteus * gblCorrection,
          transformCovariance(reorder.toProteus, gblCovariance));
    }

    // debug output of the full trajectory w/ input and results
    // this is intentionally separate from the logic to be able to show input
    // and results for each sensor together and to avoid cluttering the code.

    DEBUG("global reference: ", track.globalState());

    for (unsigned int ipoint = 0; ipoint < gblPoints.size(); ++ipoint) {
      auto sensorId = m_propagationIds[ipoint];
      auto label = ipoint + 1;
      const auto& point = gblPoints[ipoint];

      DEBUG("sensor ", sensorId, ":");

      // propagation
      DEBUG("  jacobian:\n", format(point.getP2pJacobian()));

      // track parameters

      const auto& reference = referenceParams.col(ipoint);
      const auto& state = event.getSensorEvent(sensorId).getLocalState(itrack);
      DEBUG("  params:");
      DEBUG("    reference: ", format(reference));
      DEBUG("    correction: ", format(state.params() - reference));
      DEBUG("    covariance:\n", format(state.cov()));

      // measurement

      if (point.hasMeasurement()) {
        gbl::Matrix5d projection;
        gbl::Vector5d data;
        gbl::Vector5d prec;
        unsigned int numData;

        // only the last two coordinates are valid
        point.getMeasurement(projection, data, prec);
        traj.getMeasResults(label, numData, gblResiduals, gblErrorsMeasurements,
                            gblErrorsResiduals, gblDownWeights);

        DEBUG("  measurement:");
        DEBUG("    projection:\n",
              format(projection.bottomRightCorner<2, 2>()));
        DEBUG("    data:      ", format(data.tail<2>()));
        DEBUG("    residuals: ", format(gblResiduals));
        DEBUG("    stddev data:      ", format(gblErrorsMeasurements));
        DEBUG("    stddev residuals: ", format(gblErrorsResiduals));
        DEBUG("    weights: ", format(gblDownWeights));
      }

      // scatterer

      if (point.hasScatterer()) {
        Eigen::Matrix2d transformation;
        Eigen::Vector2d data;
        Eigen::Vector2d prec;
        unsigned int numData;

        point.getScatterer(transformation, data, prec);
        traj.getScatResults(label, numData, gblResiduals, gblErrorsMeasurements,
                            gblErrorsResiduals, gblDownWeights);

        DEBUG("  scatterer:");
        DEBUG("    transformation:\n", format(transformation));
        DEBUG("    data:      ", format(data));
        DEBUG("    residuals: ", format(gblResiduals));
        DEBUG("    stddev data:      ", format(gblErrorsMeasurements));
        DEBUG("    stddev residuals: ", format(gblErrorsResiduals));
        DEBUG("    weights: ", format(gblDownWeights));
      }
    }

    DEBUG("chi2/dof: ", chi2, " / ", dof);
    DEBUG("lost weight: ", lostWeight);

    // debug printout
    // traj.printTrajectory(1);
    // traj.printPoints(1);
    // traj.printData();
  }
}
