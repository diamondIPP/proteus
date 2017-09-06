/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "gblfitter.h"

#include "mechanics/device.h"
#include "mechanics/geometry.h"
#include "mechanics/sensor.h"
#include "storage/event.h"
#include "tracking/tracking.h"
#include "utils/logger.h"
#include <math.h>

PT_SETUP_LOCAL_LOGGER(GBLFitter)

Tracking::GBLFitter::GBLFitter(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Tracking::GBLFitter::name() const { return "GBLFitter"; }

void Tracking::GBLFitter::process(Storage::Event& event) const
{
  // Print Generic Information
  INFO("Number of tracks in the event: ", event.numTracks());

  // Declare a vector for storing the original state
  Eigen::Vector4d originalState;

  // INIT: Loop over all the sensors to get the total radiation length
  double totalRadLength = 0;
  for (Index isns = 0; isns < m_device.numSensors(); ++isns)
  {
    totalRadLength += m_device.getSensor(isns)->xX0();
  }
  DEBUG("Total Radiation Length: ", totalRadLength);

  // Loop over all the tracks in the event
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack)
  {
    // Initialize list of GblPoints
    std::vector<gbl::GblPoint> points;

    // Initialize the vector of original states
    std::vector<Eigen::Vector4d> originalStates;

    // Get the track
    Storage::Track& track = event.getTrack(itrack);

    // Print Track info
    DEBUG("TrackState: ", track.globalState());
    INFO("Number of clusters in the track: ", track.size());

    // Specify the track in global coordinates
    auto offset = track.globalState().offset();
    auto slope = track.globalState().slope();
    Eigen::Vector3d trackPos(offset.x(), offset.y(), 0);
    Eigen::Vector3d trackDirec(slope.x(), slope.y(), 1);
    trackDirec.normalize();
    DEBUG("Track Direction: ", trackDirec);

    // Loop over all the sensors
    for (Index isensor = 0; isensor < m_device.numSensors(); isensor++)
    {
      DEBUG("SensorId: ", isensor);
      DEBUG("Sensor Name: ", m_device.getSensor(isensor)->name());
      DEBUG("Sensor Rows: ", m_device.getSensor(isensor)->numRows());
      DEBUG("Sensor Cols: ", m_device.getSensor(isensor)->numCols());
      DEBUG("Sensor Pitch Row: ", m_device.getSensor(isensor)->pitchRow());
      DEBUG("Sensor Pitch Col: ", m_device.getSensor(isensor)->pitchCol());
      DEBUG("Sensor Origin: ", m_device.getSensor(isensor)->origin());
      DEBUG("Sensor Normal: ", m_device.getSensor(isensor)->normal());
      DEBUG("Sensor L2G: ", m_device.getSensor(isensor)->localToGlobal());
      DEBUG("Sensor G2L: ", m_device.getSensor(isensor)->globalToLocal());
      DEBUG("Sensor P2G: ", m_device.getSensor(isensor)->constructPixelToGlobal());

      // Get the Plane Normal vector
      Eigen::Vector3d planeNormal;
      planeNormal(0) = m_device.getSensor(isensor)->normal().X();
      planeNormal(1) = m_device.getSensor(isensor)->normal().Y();
      planeNormal(2) = m_device.getSensor(isensor)->normal().Z();
      DEBUG("Plane Normal: ", planeNormal);

      // Propagate global track to intersection to get global path length
      // to the intersection point between track and sensor
      double xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz;
      m_device.getSensor(isensor)->localToGlobal().GetComponents(
        xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz);
      Eigen::Vector3d localOrigin(dx, dy, dz);
      double pathLength = -1 * (trackPos - localOrigin).dot(planeNormal) / trackDirec.dot(planeNormal);
      DEBUG("Path Length: ", pathLength);
      // Get the global intersection coordinates
      Eigen::Vector3d globalIntersection;
      globalIntersection = trackPos + pathLength * trackDirec;

       // Compute Q1 from the G2L matrix of the sensor
       // NOTE: This would also be used for computing the Jacobian
       // TODO: Change to iterator version
       Eigen::Matrix3d Q1;
       Eigen::Matrix3d Q1T;
       Eigen::MatrixXd Q1T23(2,3);
       m_device.getSensor(isensor)->globalToLocal().GetComponents(
         xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz);
       Q1(0,0) = xx;
       Q1(0,1) = xy;
       Q1(0,2) = xz;
       Q1(1,0) = yx;
       Q1(1,1) = yy;
       Q1(1,2) = yz;
       Q1(2,0) = zx;
       Q1(2,1) = zy;
       Q1(2,2) = zz;
       // Get the Q1 transpose
       Q1T = Q1.transpose();
       // Get the Q1T23 block
       Q1T23 = Q1T.block(0,0,2,3);

      // Convert position into local coordinates
      Eigen::Vector3d localIntersection = Q1 * (globalIntersection -
        localOrigin);
      // Convert track direction into local tangents
      Eigen::Vector3d localTangent = Q1 * trackDirec;
      // Convert local tangents to local slopes
      Eigen::Vector2d localSlope;
      localSlope(0) = localTangent(0) / localTangent(2);
      localSlope(1) = localTangent(1) / localTangent(2);

      // Initialize the state vector
      originalState(0) = localSlope(0);
      originalState(1) = localSlope(1);
      originalState(2) = localIntersection(0);
      originalState(3) = localIntersection(1);
      // Push back the original state to the vectors
      originalStates.push_back(originalState);

      DEBUG("Original State: ", originalState);
      DEBUG("Track Intersection with sensor in Global coordinates: ",
        globalIntersection);
      DEBUG("Track Intersection with sensor in Local coordinates: ",
        localIntersection);
      DEBUG("Local tangent: ", localTangent);
      DEBUG("Local slope: ", localSlope);

      // Caluclate Jacobians for all the sensors
      // TODO: Opportunity to optimize code by using fixed size matrices below
      Eigen::Matrix3d Q0;
      if (isensor == 0)
      {
        // Compute Q0 from L2G matrix
        m_device.getSensor(isensor)->localToGlobal().GetComponents(
          xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz);
        Q0(0,0) = xx;
        Q0(0,1) = xy;
        Q0(0,2) = xz;
        Q0(1,0) = yx;
        Q0(1,1) = yy;
        Q0(1,2) = yz;
        Q0(2,0) = zx;
        Q0(2,1) = zy;
        Q0(2,2) = zz;
      }
      else
      {
        // Compute Q0 from L2G matrix of the previous sensor
        double xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz;
        m_device.getSensor(isensor - 1)->localToGlobal().GetComponents(
          xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz);
        Q0(0,0) = xx;
        Q0(0,1) = xy;
        Q0(0,2) = xz;
        Q0(1,0) = yx;
        Q0(1,1) = yy;
        Q0(1,2) = yz;
        Q0(2,0) = zx;
        Q0(2,1) = zy;
        Q0(2,2) = zz;
      }

      // Compute Matrix A
      float f;
      f = 1 / sqrt(1 + localSlope(0)*localSlope(0) +
        localSlope(1)*localSlope(1));
      float f_cubed;
      f_cubed = f*f*f;
      Eigen::MatrixXd A(3,2);
      A(0,0) = f - f_cubed * localSlope(0)*localSlope(0);
      A(0,1) = -f * localSlope(0) * localSlope(1);
      A(1,0) = A(0,1);
      A(1,1) = f - f_cubed * localSlope(1)*localSlope(1);
      A(2,0) = -f_cubed * localSlope(0);
      A(2,1) = -f_cubed * localSlope(1);

      // Compute Matrix F (Local to global)
      Eigen::MatrixXd F_0(3,2);
      F_0 = Q0.block(0,0,3,2);
      Eigen::MatrixXd F_1 = Eigen::MatrixXd::Zero(3, 2);
      Eigen::MatrixXd F_2 = Q0 * A;
      Eigen::MatrixXd F_3(F_0.rows(), F_0.cols() + F_1.cols());
      F_3 << F_0, F_1;
      Eigen::MatrixXd F_4(F_1.rows(), F_1.cols() + F_2.cols());
      F_4 << F_1, F_2;
      Eigen::MatrixXd F(F_3.rows() + F_4.rows(), F_3.cols());
      F << F_3, F_4;

      // Compute Matrix B
      Eigen::Matrix3d twt = trackDirec * planeNormal.transpose();
      double tw = trackDirec.dot(planeNormal);
      Eigen::Matrix3d eye3 = Eigen::Matrix3d::Identity();
      Eigen::Matrix3d B = eye3 - (twt/tw);

      //Computer Matrix C
      // TODO: Check which path length I should use
      Eigen::Matrix3d ttt = trackDirec * trackDirec.transpose();
      Eigen::Matrix3d C = pathLength * (eye3 - (ttt/tw));

      // Compute Matrix D
      Eigen::MatrixXd D(2,3);
      D(0,0) = f;
      D(0,1) = 0;
      D(0,2) = -localSlope(0);
      D(1,0) = D(0,1);
      D(1,1) = D(0,0);
      D(1,2) = -localSlope(1);

      // Compute Matrix G
      Eigen::MatrixXd G_0(2,3);
      G_0 = Q1T23 * B;
      Eigen::MatrixXd G_1(2,3);
      G_1 = Q1T23 * C;
      Eigen::MatrixXd G_2 = Eigen::MatrixXd::Zero(2, 3);
      Eigen::MatrixXd G_3(2,3);
      G_3 = D * Q1T;
      Eigen::MatrixXd G_4(G_0.rows(), G_0.cols() + G_1.cols());
      G_4 << G_0, G_1;
      Eigen::MatrixXd G_5(G_2.rows(), G_2.cols() + G_3.cols());
      G_5 << G_2, G_3;
      Eigen::MatrixXd G(G_4.rows() + G_5.rows(), G_4.cols());
      G << G_4, G_5;

      // Compute Matrix H
      Eigen::Matrix4d H_temp = G * F;

      // Rearrange H so that it corresponds to (u',v',u,v) instead of
      // (u,v,u',v')
      Eigen::MatrixXd H1(2,2);
      H1 = H_temp.block(0,0,2,2);
      Eigen::MatrixXd H2(2,2);
      H2 = H_temp.block(0,2,2,2);
      Eigen::MatrixXd H3(2,2);
      H3 = H_temp.block(2,0,2,2);
      Eigen::MatrixXd H4(2,2);
      H4 = H_temp.block(2,2,2,2);
      Eigen::MatrixXd H5(2,4);
      H5 << H4, H3;
      Eigen::MatrixXd H6(2,4);
      H6 << H2, H1;
      Eigen::Matrix4d H;
      H << H5, H6;

      // We have to make the Jacobain 5x5 in GBL format (q/p,u',v',u,v)
      Eigen::MatrixXd jac_0(1,4);
      jac_0(0,0) = 0;
      jac_0(0,1) = 0;
      jac_0(0,2) = 0;
      jac_0(0,3) = 0;
      Eigen::MatrixXd jac_1(5,4);
      jac_1 << jac_0, H;
      Eigen::MatrixXd jac_2(5,1);
      jac_2(0,0) = 1;
      jac_2(1,0) = 0;
      jac_2(2,0) = 0;
      jac_2(3,0) = 0;
      jac_2(4,0) = 0;
      Eigen::MatrixXd jac(5,5);
      jac << jac_2, jac_1;

      // Print the Jacobian information
      DEBUG("Q0: ", Q0);
      DEBUG("f: ", f);
      DEBUG("f^3: ", f_cubed);
      DEBUG("A: ", A);
      DEBUG("F: ", F);
      DEBUG("Q1: ", Q1);
      DEBUG("Q1 Transpose: ", Q1T);
      DEBUG("TWT: ", twt);
      DEBUG("TW: ", tw);
      DEBUG("Eye3: ", eye3);
      DEBUG("B: ", B);
      DEBUG("Path Length: ", pathLength);
      DEBUG("TTT: ", ttt);
      DEBUG("C: ", C);
      DEBUG("D: ", D);
      DEBUG("G: ", G);
      DEBUG("H: ", H);
      DEBUG("Jacobian: ", jac);

      // Initialize the GBL point with the Jacobian
      gbl::GblPoint point(jac);

      //Get theta from the Highland formula
      // TODO: Seems like we need to adapt this formula for our case
      // Need energy for the calculation
      double radLength = m_device.getSensor(isensor)->xX0();
      double energy = 180;
      double theta = 0.0136 * sqrt(radLength) / energy * (1
        + 0.038 * log(totalRadLength));

      // Get the scattering uncertainity
      Eigen::Vector2d scatPrec;
      scatPrec(0) = 1 / (theta * theta);
      scatPrec(1) = scatPrec(0);

      // Set the scattering mean as Zero
      Eigen::Vector2d scat;
      scat(0) = 0;
      scat(1) = 0;

      // Add scatterer to the defined GblPoint
      point.addScatterer(scat, scatPrec);

      // Loop over all the clusters to find the matching information
      for (const auto& c : track.clusters())
      {
        Index isensorFromCluster = c.first;
        const Storage::Cluster& cluster = c.second;
        if (isensor == isensorFromCluster)
        {
          DEBUG("Cluster ", cluster);
          DEBUG("Sensor Number of Hits: ", cluster.size());
          for (const Storage::Hit& someHit : cluster.hits()) {
            DEBUG("Hit: ", someHit);
          }
          // Get the measurement precision
          Eigen::Matrix2d measCoVar;
          measCoVar(0,0) = cluster.covLocal()(0,0);
          measCoVar(0,1) = cluster.covLocal()(0,1);
          measCoVar(1,0) = cluster.covLocal()(1,0);
          measCoVar(1,1) = cluster.covLocal()(1,1);
          Eigen::Matrix2d measPrec;
          measPrec = measCoVar.inverse();
          DEBUG("measPrec: ", measPrec);

          // Get the measurement (residuals)
          Eigen::Vector2d meas;
          meas(0) = localIntersection(0) - cluster.posLocal().x();
          meas(1) = localIntersection(1) - cluster.posLocal().y();
          DEBUG("Meas: ", meas);

          // Set the proL2m matrix to unit matrix
          // Identity Matrix since measurement and scattering plane are the same
          Eigen::Matrix2d proL2m = Eigen::Matrix2d::Identity();

          // Add measurement to the defined GblPoint
          point.addMeasurement(proL2m, meas, measPrec);
        }
      }

      // Add the GblPoint to the list of GblPoints
      points.push_back(point);
    }

    //Define and initialize the GblTrajectory
    gbl::GblTrajectory traj(points, false);

    // Fit trajectory
    double Chi2;
    int Ndf;
    double lostWeight;
    traj.fit(Chi2, Ndf, lostWeight);

    INFO("Chi2: ", Chi2);
    INFO("Ndf: ", Ndf);
    INFO("lostWeight: ", lostWeight);

    // Loop over all the sensors to get the measurement and scatterer
    // residuals and then update the track state
    for (Index isensor = 0; isensor < m_device.numSensors(); ++isensor)
    {
      unsigned int numData = 0;
      Eigen::VectorXd residuals(2), measErr(2), resErr(2), downWeights(2);

      // Get the measurement residuals
      traj.getMeasResults(isensor + 1, numData, residuals, measErr, resErr,
      downWeights);
      DEBUG("Measurement Results for Sensor ", isensor);
      for (unsigned int i = 0; i < numData; ++i)
      {
        DEBUG(i, " Residual: ", residuals[i], " , Measurement Error: ",
        measErr[i], " , Residual Error: ", resErr[i]);
      }

      // Get the scatterer residuals
      Eigen::VectorXd sc1(2), sc2(2), sc3(2), sc4(2);
      traj.getScatResults(isensor + 1, numData, sc1, sc2, sc3, sc4);
      DEBUG("Scattering Results for Sensor ", isensor);
      for (unsigned int i = 0; i < numData; ++i)
      {
        // TODO: Revise these names. Are probably incorrect.
        DEBUG(i, " Kink: ", sc1[i], " , Kink measurement error: ", sc2[i],
         " , Kink error: ", sc3[i]);
      }

      // Get the track parameter corrections
      Eigen::VectorXd aCorrection(5);
      Eigen::MatrixXd aCovariance(5,5);
      traj.getResults(isensor + 1, aCorrection, aCovariance);
      DEBUG("aCorrection: ", aCorrection);
      DEBUG("Covariance: ", aCovariance);

      // Calcualte the updated state: u', v', u, v
      DEBUG("For Sensor: ", isensor);
      Eigen::Vector4d corr = aCorrection.tail(4);
      DEBUG("Correction: ", corr);
      Eigen::Vector4d updatedState = originalStates[isensor] - corr;
      DEBUG("Original State: ", originalStates[isensor]);
      DEBUG("Updated State: ", updatedState);

      // Initialize and update the sensor event for each sensor
      Storage::SensorEvent& sev = event.getSensorEvent(isensor);
      Storage::TrackState state(updatedState(2), updatedState(3),
        updatedState(0), updatedState(1));

      // Update the covariance for each state
      Eigen::Matrix4d cov = aCovariance.block(1,1,4,4);
      state.setCovMat(cov);

      // Set the local state for the sensor event
      sev.setLocalState(itrack, std::move(state));
    }

    // debug printout
	  //traj.printTrajectory(1);
		//traj.printPoints(1);
		//traj.printData();
  }
}
