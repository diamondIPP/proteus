/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "straightfitter.h"

#include <limits>

#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/linefitter.h"

// Fit the track in the local target plane
static void fitLocal(const Storage::Track& track,
                     const Mechanics::Geometry& geo,
                     const Mechanics::Plane& target,
                     Storage::TrackState& state,
                     Scalar& chi2,
                     int& dof,
                     Index ignoreSensorId = kInvalidIndex)
{
  Tracking::LineFitter3D fitter;
  auto time = std::numeric_limits<Scalar>::max();

  for (const auto& ci : track.clusters()) {
    // only add the cluster if it is not ignored
    if (ci.first == ignoreSensorId) {
      continue;
    }
    const auto& source = geo.getPlane(ci.first);
    const Storage::Cluster& cluster = ci.second;

    // source local to target local assuming position on the local source plane
    Vector4 local = target.toLocal(source.toGlobal(cluster.position()));
    Matrix4 jac = target.linearToLocal() * source.linearToGlobal();
    Vector4 weight = transformCovariance(jac, cluster.positionCov())
                         .diagonal()
                         .cwiseInverse();
    fitter.addPoint(local, weight);

    // the track time is not fitted; fasted time is selected for now
    time = std::min(time, cluster.time());
  }

  fitter.fit();
  state = Storage::TrackState(fitter.params(), fitter.cov());
  chi2 = fitter.chi2();
  dof = fitter.dof();
}

Tracking::StraightFitter::StraightFitter(const Mechanics::Device& device)
    : m_geo(device.geometry()), m_sensorIds(device.sensorIds())
{
}

std::string Tracking::StraightFitter::name() const { return "StraightFitter"; }

void Tracking::StraightFitter::execute(Storage::Event& event) const
{
  Storage::TrackState state;
  Scalar chi2;
  int dof;

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitLocal(track, m_geo, Mechanics::Plane{}, state, chi2, dof);
    track.setGlobalState(state);
    track.setGoodnessOfFit(chi2, dof);

    // local fit for optimal parameters/covariance on each sensor plane
    for (Index iref : m_sensorIds) {
      fitLocal(track, m_geo, m_geo.getPlane(iref), state, chi2, dof);
      event.getSensorEvent(iref).setLocalState(itrack, state);
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
  Storage::TrackState state;
  Scalar chi2;
  int dof;

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitLocal(track, m_geo, Mechanics::Plane{}, state, chi2, dof);
    track.setGlobalState(state);
    track.setGoodnessOfFit(chi2, dof);

    // local fit for optimal parameters/covariance on each sensor plane
    for (Index iref : m_sensorIds) {
      fitLocal(track, m_geo, m_geo.getPlane(iref), state, chi2, dof, iref);
      event.getSensorEvent(iref).setLocalState(itrack, state);
    }
  }
}
