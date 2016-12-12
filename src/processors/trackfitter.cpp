/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "trackfitter.h"

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Processors::StraightTrackFitter::StraightTrackFitter(
    const Mechanics::Device& device, const std::vector<Index>& sensorIds)
    : m_device(device), m_sensorIds(std::begin(sensorIds), std::end(sensorIds))
{
  DEBUG("fit on sensors: ", m_sensorIds);
}

std::string Processors::StraightTrackFitter::name() const
{
  return "StraightTrackFitter";
}

void Processors::StraightTrackFitter::process(Storage::Event& event) const
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);

    for (auto id = m_sensorIds.begin(); id != m_sensorIds.end(); ++id) {
      const Mechanics::Sensor& sensor = *m_device.getSensor(*id);
      Storage::Plane& plane = *event.getPlane(*id);

      Storage::TrackState state = fitTrackLocal(track, sensor);
      state.setTrack(&track);
      plane.addState(std::move(state));
    }
  }
}
