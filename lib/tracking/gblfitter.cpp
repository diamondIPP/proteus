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

bool firstTimeFlag = true;

// Initialize list of GblPoints
std::vector<gbl::GblPoint> points;

/** Fit a GBL trajectory */
struct SimpleGBLFitter {
  // TODO
  // Derive Jacobians
  // Define Jacobians according to the GBL Example
  // Define the procedure to run the propagation similar to the Example
};

//void Tracking::getJacobians(const Mechanics::Device& device)

void Tracking::fitTrackGBL(Storage::Track& track)
{
  //SimpleGBLFitter fit;
  INFO("fitTrackGlobalGBL running");
  INFO("Number of clusters in the track: ", track.size());
  for (const auto& c : track.clusters()) {
    Index isensor = c.first;
    const Storage::Cluster& cluster = c.second;
    INFO("Cluster ", cluster);
    INFO("SensorId: ", isensor);
    INFO("Position Global: ", cluster.posGlobal());
    INFO("Position Local: ", cluster.posLocal());
    INFO("Position Pixel: ", cluster.posPixel());
    INFO("Covariance: ");
    INFO(cluster.covGlobal());
    INFO("Number of Hits: ", cluster.size());
    for (const Storage::Hit& someHit : cluster.hits()) {
      INFO("Hit: ", someHit);
    //points.push_back()
    }
  }
}

Tracking::GBLFitter::GBLFitter(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Tracking::GBLFitter::name() const { return "GBLFitter"; }

void Tracking::GBLFitter::process(Storage::Event& event) const
{
  INFO("First time flag: ", firstTimeFlag);
  INFO("gblfitter.cpp running");
  INFO("Number of tracks in the event: ", event.numTracks());

  // Specify the initial track direction
  Eigen::Vector3d trackDirec;
  double dU;
  double dV;
  if(firstTimeFlag) {
    trackDirec(0) = 0.0;
    trackDirec(1) = 0.0;
    trackDirec(2) = 1.0;
    dU = 0;
    dV = 0;
  }
  INFO("Track Direction: ", trackDirec);

  // Get the Jacobians based on geometry
  for (Index isensor = 0; isensor < m_device.numSensors(); ++isensor) {
    if(firstTimeFlag) {
      INFO("Sensor Id: ", isensor);
      INFO("Sensor Name: ", m_device.getSensor(isensor)->name());
      //INFO("Sensor Rows: ", m_device.getSensor(isensor)->numRows());
      //INFO("Sensor Cols: ", m_device.getSensor(isensor)->numCols());
      INFO("Sensor Pitch Row: ", m_device.getSensor(isensor)->pitchRow());
      INFO("Sensor Pitch Col: ", m_device.getSensor(isensor)->pitchCol());
      //INFO("Sensor Origin: ", m_device.getSensor(isensor)->origin());
      //INFO("Sensor Normal: ", m_device.getSensor(isensor)->normal());
      //INFO("Sensor Measurement: ", m_device.getSensor(isensor)->measurement());
      //INFO("Sensor L2G: ", m_device.getSensor(isensor)->localToGlobal());
      //INFO("Sensor G2L: ", m_device.getSensor(isensor)->globalToLocal());
      //INFO("Sensor P2G: ", m_device.getSensor(isensor)->constructPixelToGlobal());

      // Get the Plane Normal vector
      Eigen::Vector3d planeNormal;
      planeNormal(0) = m_device.getSensor(isensor)->normal().X();
      planeNormal(1) = m_device.getSensor(isensor)->normal().Y();
      planeNormal(2) = m_device.getSensor(isensor)->normal().Z();

      //INFO("Plane Normal: ", planeNormal);

      // Caluclate Jacobians, skipping the last sensor
      // TODO: Opportunity to optimize code by using fixed size matrices below
      if (isensor < m_device.numSensors() - 1)
      {
          // Compute Q0 from L2G matrix
          double xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz;
          m_device.getSensor(isensor)->localToGlobal().GetComponents(
            xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz);
          Eigen::Matrix3d Q0;
          Q0(0,0) = xx;
          Q0(0,1) = xy;
          Q0(0,2) = xz;
          Q0(1,0) = yx;
          Q0(1,1) = yy;
          Q0(1,2) = yz;
          Q0(2,0) = zx;
          Q0(2,1) = zy;
          Q0(2,2) = zz;

          // Compute Matrix A
          float f;
          f = 1 / sqrt(1 + dU*dU + dV*dV);
          float f_cubed;
          f_cubed = f*f*f;
          Eigen::MatrixXd A(3,2);
          A(0,0) = f - f_cubed * dU*dU;
          A(0,1) = -f * dU * dV;
          A(1,0) = A(0,1);
          A(1,1) = f - f_cubed * dV*dV;
          A(2,0) = -f_cubed * dU;
          A(2,1) = -f_cubed * dV;

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

          // Compute Q1 from the G2L matrix of the next sensor
          m_device.getSensor(isensor + 1)->globalToLocal().GetComponents(
            xx, xy, xz, dx, yx, yy, yz, dy, zx, zy, zz, dz);
          Eigen::Matrix3d Q1;
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
          Eigen::Matrix3d Q1T = Q1.transpose();
          // Get the Q1T23 block
          Eigen::MatrixXd Q1T23(2,3);
          Q1T23 = Q1T.block(0,0,2,3);

          // Compute Matrix B
          Eigen::Matrix3d twt = trackDirec * planeNormal.transpose();
          double tw = trackDirec.dot(planeNormal);
          Eigen::Matrix3d eye3 = Eigen::Matrix3d::Identity();
          Eigen::Matrix3d B = eye3 - (twt/tw);

          //Computer Matrix C
          double pathLength;
          if(isensor == 0) {
            pathLength = m_device.getSensor(isensor)->origin().Z();
          }
          else {
            pathLength = m_device.getSensor(isensor)->origin().Z() - m_device.getSensor(isensor - 1)->origin().Z();
          }
          Eigen::Matrix3d ttt = trackDirec * trackDirec.transpose();
          Eigen::Matrix3d C = pathLength * (eye3 - (ttt/tw));

          // Compute Matrix D
          Eigen::MatrixXd D(2,3);
          D(0,0) = f;
          D(0,1) = 0;
          D(0,2) = -dU;
          D(1,0) = D(0,1);
          D(1,1) = D(0,0);
          D(1,2) = -dV;

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

          // Compute Matrix H (The complete Jacobian)
          Eigen::Matrix4d H = G * F;

          // Print the Jacobian information
          // INFO("Q0: ", Q0);
          // INFO("f: ", f);
          // INFO("f^3: ", f_cubed);
          // INFO("A: ", A);
          // INFO("F: ", F);
          // INFO("Q1: ", Q1);
          // INFO("Q1 Transpose: ", Q1T);
          // INFO("TWT: ", twt);
          // INFO("TW: ", tw);
          // INFO("Eye3: ", eye3);
          // INFO("B: ", B);
          // INFO("Path Length: ", pathLength);
          // INFO("TTT: ", ttt);
          // INFO("C: ", C);
          // INFO("D: ", D);
          // INFO("G: ", G);
          //INFO("H: ", H);

          // We have to make the Jacobain 5x5
          Eigen::Vector4d jac_0(4,1);
          jac_0(0) = 0;
          jac_0(1) = 0;
          jac_0(2) = 0;
          jac_0(3) = 0;
          Eigen::MatrixXd jac_1(4,5);
          jac_1 << H, jac_0;
          Eigen::MatrixXd jac_2(1,5);
          jac_2(0,0) = 0;
          jac_2(0,1) = 0;
          jac_2(0,2) = 0;
          jac_2(0,3) = 0;
          jac_2(0,4) = 1;
          Eigen::MatrixXd jac(5,5);
          jac << jac_1, jac_2;

          INFO("Jacobian: ", jac);

          // Initialize the GBL point with the Jacobian
          gbl::GblPoint point(jac);
      }

      // Get the measurement uncertainities for the sensor
      // TODO: Change this to the more accurate formula as per PDG/Simon's code
      //double sigma_u_sqr, sigma_v_sqr;
      //sigma_u_sqr = m_device.getSensor(isensor)->pitchCol() * m_device.getSensor(isensor)->pitchCol() / 12;
      //sigma_v_sqr = m_device.getSensor(isensor)->pitchRow() * m_device.getSensor(isensor)->pitchRow() / 12;

      //Get theta from the Highland formula
      // TODO: Seems like we need to adapt this formula for our case
      // Need energy and particle charge for the calculation
      // Get the radiation length for the sensor
      double radLength = m_device.getSensor(isensor)->xX0();
      double energy = 180000;
      double theta = 0.0136 * sqrt(radLength) / energy * (1 + 0.038 * log(radLength));

      // Get the scattering uncertainity
      Eigen::Vector2d scat;
      scat(0) = 1 / (theta * theta);
      scat(1) = scat(0);

      //


      // Initialize the list of GBLPoints
      //std::vector<gbl::GblPoint> points;

      // Initialize a GBL point for each m



      // See how you can update dU and other stuff in next iterations
      // Also, what exactly should be their value?
      // After that, try to get the measurement on each sensor
      // Also the uncertainity (pixel size /12 I think)
      // And then you can define a GBL point

      // Qs: How to incorporate sensor offsets in x and y direction
      // in the Jacobians?
      // How exactly is the path length defined?
      // Also have to emit the DUT from the reconstruction part
    }
  }

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {

    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitTrackGBL(track);

    // 1. loop over sensors to build trajectory
    // std::vector<gbl::points> points
    // 1.a. for each point: jacobian, scattering uncertaint, measurement
    // 2. fit full trajectory
    // gbl::Trajectory traj(points);
    // trja.fit()
    // 3. loop over trajectory results
    ///   for each sensor
    /// 3.a get corrections
    /// 3.b calculate new TrackState
    /// 3.c set per-sensor local state
    ///     sensorEvent.setLocalState(track_id, state);

    for (Index isensor = 0; isensor < m_device.numSensors(); ++isensor) {
      Storage::SensorEvent& sev = event.getSensorEvent(isensor);
      // local fit for correct errors in the local frame
      Storage::TrackState state =
          fitTrackLocal(track, m_device.geometry(), isensor);
      sev.setLocalState(itrack, std::move(state));
      INFO("TrackState: ", state);
    }
  }
  firstTimeFlag = false;
}

/*    for (auto sensorId : m_sensorIds) {
      Storage::SensorEvent& sensorEvent = event.getSensorEvent(sensorId);
      // local fit for correct errors in the local frame
      Storage::TrackState state =
          fitTrackLocal(track, m_device.geometry(), sensorId);
      sensorEvent.setLocalState(sensorId, std::move(state));
    }
  }
}*/
