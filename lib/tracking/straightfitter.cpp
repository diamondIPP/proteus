/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "straightfitter.h"

#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/tracking.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Tracking::StraightFitter::StraightFitter(const Mechanics::Device& device,
                                         const std::vector<Index>& sensorIds)
    : m_device(device), m_sensorIds(std::begin(sensorIds), std::end(sensorIds))
{
  DEBUG("fit on sensors: ", m_sensorIds);
}

std::string Tracking::StraightFitter::name() const { return "StraightFitter"; }

void Tracking::StraightFitter::process(Storage::Event& event) const
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitTrackGlobal(track);

    for (auto sensorId : m_sensorIds) {
      Storage::SensorEvent& sensorEvent = event.getSensorEvent(sensorId);
      // local fit for correct errors in the local frame
      Storage::TrackState state =
          fitTrackLocal(track, m_device.geometry(), sensorId);
      state.setTrack(itrack);
      sensorEvent.addState(std::move(state));
    }
  }
}
