/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "straightfitter.h"

#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/linefitter.h"

Tracking::StraightFitter::StraightFitter(const Mechanics::Device& device)
    : m_geo(device.geometry()), m_sensorIds(device.sensorIds())
{
}

std::string Tracking::StraightFitter::name() const { return "StraightFitter"; }

void Tracking::StraightFitter::execute(Storage::Event& event) const
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitStraightTrackGlobal(m_geo, track);

    // local fit for optimal parameters/covariance on each sensor plane
    for (Index iref : m_sensorIds) {
      const Mechanics::Plane& ref = m_geo.getPlane(iref);

      LineFitter3D fitter;
      for (const auto& c : track.clusters())
        fitter.addPoint(c.second, m_geo.getPlane(c.first), ref);
      fitter.fit();
      event.getSensorEvent(iref).setLocalState(itrack, fitter.state());
    }
  }
}

Tracking::UnbiasedStraightFitter::UnbiasedStraightFitter(
    const Mechanics::Device& device)
    : m_geo(device.geometry()), m_sensorIds(device.sensorIds())
{
}

std::string Tracking::UnbiasedStraightFitter::name() const
{
  return "UnbiasedStraightFitter";
}

void Tracking::UnbiasedStraightFitter::execute(Storage::Event& event) const
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitStraightTrackGlobal(m_geo, track);

    // local fit for optimal parameters/covariance on each sensor plane
    for (Index iref : m_sensorIds) {
      const Mechanics::Plane& ref = m_geo.getPlane(iref);

      LineFitter3D fitter;
      for (const auto& c : track.clusters()) {
        if (c.first == iref)
          continue;
        fitter.addPoint(c.second, m_geo.getPlane(c.first), ref);
      }
      fitter.fit();
      event.getSensorEvent(iref).setLocalState(itrack, fitter.state());
    }
  }
}
