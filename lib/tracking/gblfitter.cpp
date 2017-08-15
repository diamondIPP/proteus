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
#include <GblTrajectory.h>


PT_SETUP_LOCAL_LOGGER(GBLFitter)

bool firstTimeFlag = true;

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
  if(firstTimeFlag) {
    trackDirec(0) = 0.0;
    trackDirec(1) = 0.0;
    trackDirec(2) = 1.0;
    double dU = 0;
    double dV = 0;
  }
  INFO("Track Direction: ", trackDirec);

  // Get the Jacobians based on geometry
  for (Index isensor = 0; isensor < m_device.numSensors(); ++isensor) {
    if(firstTimeFlag) {
      INFO("Sensor Id: ", isensor);
      INFO("Sensor Name: ", m_device.getSensor(isensor)->name());
      INFO("Sensor Rows: ", m_device.getSensor(isensor)->numRows());
      INFO("Sensor Cols: ", m_device.getSensor(isensor)->numCols());
      INFO("Sensor Pitch Row: ", m_device.getSensor(isensor)->pitchRow());
      INFO("Sensor Pitch Col: ", m_device.getSensor(isensor)->pitchCol());
      INFO("Sensor Origin: ", m_device.getSensor(isensor)->origin());
      INFO("Sensor Normal: ", m_device.getSensor(isensor)->normal());
      //INFO("Sensor Measurement: ", m_device.getSensor(isensor)->measurement());
      INFO("Sensor L2G: ", m_device.getSensor(isensor)->localToGlobal());
      INFO("Sensor G2L: ", m_device.getSensor(isensor)->globalToLocal());
      INFO("Sensor P2G: ", m_device.getSensor(isensor)->constructPixelToGlobal());

      // Get the Plane Normal vector
      Eigen::Vector3d planeNormal;
      planeNormal(0) = m_device.getSensor(isensor)->normal().X();
      planeNormal(1) = m_device.getSensor(isensor)->normal().Y();
      planeNormal(2) = m_device.getSensor(isensor)->normal().Z();

      INFO("Plane Normal: ", planeNormal);

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


      INFO("TWT: ", twt);
      INFO("TW: ", tw);
      INFO("Eye3: ", eye3);
      INFO("B: ", B);
      INFO("Path Length: ", pathLength);
      INFO("TTT: ", ttt);
      INFO("C: ", C);
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
