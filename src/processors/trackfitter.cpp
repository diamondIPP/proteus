/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "trackfitter.h"

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/event.h"

std::string Processors::StraightTrackFitter::name() const
{
  return "StraightTrackFitter";
}

void Processors::StraightTrackFitter::process(Storage::Event& event) const
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = *event.getTrack(itrack);

    for (auto id = m_sensorIds.begin(); id != m_sensorIds.end(); ++id) {
      const Mechanics::Sensor& sensor = *m_device.getSensor(*id);
      track.addLocalState(*id, fitTrackLocal(track, sensor));
    }
  }
}
