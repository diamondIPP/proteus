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
    }

    // Get the Jacobians for propagation

    //points.push_back()
    // fit.addPoint(cluster.posGlobal(), cluster.covGlobal());
  }
  // fit.fit();
  // track.setGlobalState(fit.state());
  // track.setGoodnessOfFit(fit.chi2(), 2 * (track.numClusters() - 2));
}

Tracking::GBLFitter::GBLFitter(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Tracking::GBLFitter::name() const { return "GBLFitter"; }

void Tracking::GBLFitter::process(Storage::Event& event) const
{
  INFO("gblfitter.cpp running");
  INFO("Number of tracks in the event: ", event.numTracks());

  // Get the Jacobians based on geometry
  for (Index isensor = 0; isensor < m_device.numSensors(); ++isensor) {
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
  }
  //m_device.print();

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
      //INFO(m_device.name());
      // local fit for correct errors in the local frame
      Storage::TrackState state =
          fitTrackLocal(track, m_device.geometry(), isensor);
      sev.setLocalState(itrack, std::move(state));
    }
  }
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
