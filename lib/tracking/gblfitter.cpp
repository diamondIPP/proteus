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

PT_SETUP_LOCAL_LOGGER(GBLFitter)

Tracking::GBLFitter::GBLFitter(const Mechanics::Device& device)
    : m_device(device)
    // sensor ids sorted along the expected propagation order
    , m_propagationIds(
          Mechanics::sortedAlongBeam(m_device.geometry(), m_device.sensorIds()))
{
}

std::string Tracking::GBLFitter::name() const { return "GBLFitter"; }

/// Construct a GBL Jacobian for the propagation between two planes.
///
/// \param tangent   Track tangent vector in the source system
/// \param toTarget  Transformation matrix from the source to the target system
/// \param w         Propagation distance along the target normal axis
///
static Matrix5
buildJacobian(const Vector4& tangent, const Matrix4& toTarget, Scalar w)
{
  // Full Jacobian w/ Proteus parameter ordering
  Matrix6 jacPt = Tracking::jacobianState(tangent, toTarget, w);
  // Reduce to Jacobian w/ GBL ordering [q/p, u', v', u, v] and no time
  Matrix5 jacGbl = Matrix5::Zero();
  jacGbl(0, 0) = 1;
  jacGbl(1, 1) = jacPt(kSlopeLoc0, kSlopeLoc0);
  jacGbl(1, 2) = jacPt(kSlopeLoc0, kSlopeLoc1);
  jacGbl(1, 3) = jacPt(kSlopeLoc0, kLoc0);
  jacGbl(1, 4) = jacPt(kSlopeLoc0, kLoc1);
  jacGbl(2, 1) = jacPt(kSlopeLoc1, kSlopeLoc0);
  jacGbl(2, 2) = jacPt(kSlopeLoc1, kSlopeLoc1);
  jacGbl(2, 3) = jacPt(kSlopeLoc1, kLoc0);
  jacGbl(2, 4) = jacPt(kSlopeLoc1, kLoc1);
  jacGbl(3, 1) = jacPt(kLoc0, kSlopeLoc0);
  jacGbl(3, 2) = jacPt(kLoc0, kSlopeLoc1);
  jacGbl(3, 3) = jacPt(kLoc0, kLoc0);
  jacGbl(3, 4) = jacPt(kLoc0, kLoc1);
  jacGbl(4, 1) = jacPt(kLoc1, kSlopeLoc0);
  jacGbl(4, 2) = jacPt(kLoc1, kSlopeLoc1);
  jacGbl(4, 3) = jacPt(kLoc1, kLoc0);
  jacGbl(4, 4) = jacPt(kLoc1, kLoc1);
  return jacGbl;
}

/// Construct a full track state from reference parameters and GBL fit results.
///
/// This also handles the conversion from the GBL parameter order to the
/// Proteus parameter ordering used everywhere else.
static Storage::TrackState
buildCorrectedState(const Vector6& reference,
                    const Eigen::VectorXd& correction,
                    const Eigen::MatrixXd& covariance)
{
  // ensure code assumptions stay valid
  static_assert((kLoc0 + 1 == kLoc1), "Who changed the order?");
  static_assert((kSlopeLoc0 + 1 == kSlopeLoc1), "Who changed the order?");
  assert(correction.size() == 5);
  assert((covariance.cols() == 5) and (covariance.rows() == 5));

  Vector6 params = reference;
  // GBL ordering is [q/p, u', v', u, v]
  params[kLoc0] -= correction[3];
  params[kLoc1] -= correction[4];
  params[kSlopeLoc0] -= correction[1];
  params[kSlopeLoc1] -= correction[2];

  SymMatrix6 cov = Matrix6::Zero();
  cov.block<2, 2>(kLoc0, kLoc0) = covariance.block<2, 2>(3, 3);
  cov.block<2, 2>(kLoc0, kSlopeLoc0) = covariance.block<2, 2>(3, 1);
  cov.block<2, 2>(kSlopeLoc0, kSlopeLoc0) = covariance.block<2, 2>(1, 1);
  cov.block<2, 2>(kSlopeLoc0, kLoc0) = covariance.block<2, 2>(1, 3);

  return {params, cov};
}

void Tracking::GBLFitter::execute(Storage::Event& event) const
{
  using gbl::GblPoint;
  using gbl::GblTrajectory;

  // Print Generic Information
  INFO("Number of tracks in the event: ", event.numTracks());

  // INIT: Loop over all the sensors to get the total radiation length
  double totalRadLength = 0;
  for (Index isns = 0; isns < m_device.numSensors(); ++isns) {
    totalRadLength += m_device.getSensor(isns).xX0();
  }
  DEBUG("Total Radiation Length: ", totalRadLength);

  // temporary (resuable) storage
  std::vector<Vector6> referenceParams(m_device.numSensors());
  std::vector<GblPoint> gblPoints(m_device.numSensors(), {Matrix5::Identity()});
  Eigen::VectorXd gblCorrection(5);
  Eigen::MatrixXd gblCovariance(5, 5);
  Eigen::VectorXd gblResiduals(2);
  Eigen::VectorXd gblErrorsMeasurements(2);
  Eigen::VectorXd gblErrorsResiduals(2);
  Eigen::VectorXd gblDownWeights(2);

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);
    DEBUG("TrackState: ", track.globalState());
    INFO("Number of clusters in the track: ", track.size());

    // Clear temporary storage
    referenceParams.clear();
    gblPoints.clear();

    // Reference track in global coordinates
    Vector4 globalPos = track.globalState().position();
    Vector4 globalTan = track.globalState().tangent();
    Index prevSensorId = kInvalidIndex;

    // Propagate reference track through all sensor to define GBL trajectory
    for (auto sensorId : m_propagationIds) {
      const auto& plane = m_device.geometry().getPlane(sensorId);

      // 1. Propagate track state to the plane intersection

      // equivalent state in local parameters
      Vector4 localPos = plane.toLocal(globalPos);
      Vector4 localTan = plane.linearToLocal() * globalTan;
      // propagation distance along the plane normal
      Scalar dw = -localPos[kW];
      // convert tangent to slope parametrization
      localTan /= localTan[kW];
      // propagate position to the intersection
      localPos += dw * localTan;

      // 2. Compute local track parameters to be used as reference later on

      Vector6 params;
      params[kLoc0] = localPos[kU];
      params[kLoc1] = localPos[kV];
      params[kTime] = localPos[kS];
      // tangent is already in slope parametrization
      params[kSlopeLoc0] = localTan[kU];
      params[kSlopeLoc1] = localTan[kV];
      params[kSlopeTime] = localTan[kS];
      referenceParams.push_back(params);

      // 3. Compute Jacobian from previous to current plane
      // Note: this is already computed in the GBL parameter ordering which is
      // different from the parameter ordering in the rest of the code base.

      Matrix5 jac;
      if (prevSensorId == kInvalidIndex) {
        // first plane has no predecessor and no propagation Jacobian.
        jac = Matrix5::Identity();
      } else {
        const auto& prev = m_device.geometry().getPlane(prevSensorId);
        Vector4 prevTan = prev.linearToLocal() * globalTan;
        Matrix4 prevToLocal = plane.linearToLocal() * prev.linearToGlobal();
        jac = buildJacobian(prevTan, prevToLocal, dw);
      }

      // 4. Create GBL point

      GblPoint point(jac);

      // 4a. Always add a scatterer

      // Get theta from the Highland formula
      // TODO: Seems like we need to adapt this formula for our case
      // Need energy for the calculation
      double radLength = m_device.getSensor(sensorId).xX0();
      double energy = 180;
      double theta =
          0.0136 * sqrt(radLength) / energy * (1 + 0.038 * log(totalRadLength));

      // Get the scattering uncertainity
      // TODO this is incorrect for rotated planes
      Vector2 scatPrec;
      scatPrec(0) = 1 / (theta * theta);
      scatPrec(1) = scatPrec(0);

      // Add scatterer to the defined GblPoint
      // Expected scattering angle vanishes
      point.addScatterer(Vector2::Zero(), scatPrec);

      // 4b. If available, add a measurement

      auto ic = track.clusters().find(sensorId);
      if (ic != track.clusters().end()) {
        const Storage::Cluster& cluster = ic->second;
        DEBUG("Cluster ", cluster);
        DEBUG("Sensor Number of Hits: ", cluster.size());
        for (const Storage::Hit& someHit : cluster.hits()) {
          DEBUG("Hit: ", someHit);
        }

        // Get the measurement precision
        Matrix2 measPrec = cluster.uvCov().inverse();
        DEBUG("measPrec:\n", measPrec);

        // Get the measurement (residuals)
        Vector2 meas(localPos[kU] - cluster.u(), localPos[kV] - cluster.v());
        DEBUG("Meas: ", meas);

        // Add measurement to the point, measurements and track parameters
        // are defined in the same coordinates and no projection is required.
        point.addMeasurement(meas, measPrec);
      }

      // 5. Add GBL point to the trajectory

      gblPoints.push_back(point);

      // 6. Update global state as the starting point for the next iteration

      globalPos = plane.toGlobal(localPos);
      // the tangent is a constant of the motion and remains unchanged
    }

    // fit the GBL trajectory w/o track curvature

    GblTrajectory traj(gblPoints, false);
    double chi2;
    double lostWeight;
    int dof;
    // TODO what to do for errors?
    traj.fit(chi2, dof, lostWeight);
    track.setGoodnessOfFit(chi2, dof);

    DEBUG("chi2/dof: ", chi2, " / ", dof);
    DEBUG("lost weight: ", lostWeight);

    // extract local track states for all sensors

    int ipoint = 0;
    for (auto sensorId : m_propagationIds) {
      // GBL label starts counting at 1, w/ positive values indicating that
      // we want to get the parameters before the scatterer.
      int gblLabel = ipoint + 1;

      // Get the track parameter corrections and build the full state
      traj.getResults(gblLabel, gblCorrection, gblCovariance);
      auto state = buildCorrectedState(referenceParams[ipoint], gblCorrection,
                                       gblCovariance);
      // Set the local state for the sensor event
      event.getSensorEvent(sensorId).setLocalState(itrack, std::move(state));

      DEBUG("results sensor ", sensorId);
      DEBUG("  correction: ", format(gblCorrection));
      DEBUG("  parameters: ", format(state.params()));
      DEBUG("  covariance:\n", format(state.cov()));

      unsigned int numData = 0;
      Eigen::VectorXd residuals(2), measErr(2), resErr(2), downWeights(2);

      // Get the measurement residuals
      traj.getMeasResults(gblLabel, numData, gblResiduals,
                          gblErrorsMeasurements, gblErrorsResiduals,
                          gblDownWeights);
      DEBUG("Measurement Results for Sensor ", sensorId);
      for (unsigned int i = 0; i < numData; ++i) {
        DEBUG(i, " Residual: ", residuals[i],
              " , Measurement Error: ", measErr[i],
              " , Residual Error: ", resErr[i]);
      }

      // Get the scatterer residuals
      Eigen::VectorXd sc1(2), sc2(2), sc3(2), sc4(2);
      traj.getScatResults(gblLabel, numData, sc1, sc2, sc3, sc4);
      DEBUG("Scattering Results for Sensor ", sensorId);
      for (unsigned int i = 0; i < numData; ++i) {
        // TODO: Revise these names. Are probably incorrect.
        DEBUG(i, " Kink: ", sc1[i], " , Kink measurement error: ", sc2[i],
              " , Kink error: ", sc3[i]);
      }

      ipoint += 1;
    }

    // debug printout
    // traj.printTrajectory(1);
    // traj.printPoints(1);
    // traj.printData();
  }
}
