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
                     float& chi2,
                     int& dof,
                     Index ignoreSensorId = kInvalidIndex)
{
  Tracking::LineFitter3D fitter;
  float time = std::numeric_limits<float>::max();

  for (const auto& ci : track.clusters()) {
    // only add the cluster if it is not ignored
    if (ci.first == ignoreSensorId) {
      continue;
    }
    const auto& source = geo.getPlane(ci.first);
    const Storage::Cluster& cluster = ci.second;

    // source local to target local assuming a position on the local source plane
    Vector3 uvw = target.toLocal(source.toGlobal(cluster.posLocal()));
    // the extra eval is needed to make older Eigen/compiler versions happy
    Matrix2 jac = target.rotationToLocal().eval().topRows<2>() *
                  source.rotationToGlobal().leftCols<2>();
    SymMatrix2 cov = transformCovariance(jac, cluster.covLocal());
    fitter.addPoint(uvw[0], uvw[1], uvw[2], 1 / cov(0, 0), 1 / cov(1, 1));

    // the track time is not fitted; fasted time is selected for now
    time = std::min(time, cluster.time());
  }

  fitter.fit();
  state = Storage::TrackState(fitter.params(), time);
  state.setCov(fitter.cov());
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
  const Mechanics::Plane kGlobal; // default plane is the global plane
  Storage::TrackState state;
  float chi2;
  int dof;

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitLocal(track, m_geo, kGlobal, state, chi2, dof);
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
  const Mechanics::Plane kGlobal; // default plane is the global plane
  Storage::TrackState state;
  float chi2;
  int dof;

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Storage::Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit
    fitLocal(track, m_geo, kGlobal, state, chi2, dof);
    track.setGlobalState(state);
    track.setGoodnessOfFit(chi2, dof);

    // local fit for optimal parameters/covariance on each sensor plane
    for (Index iref : m_sensorIds) {
      fitLocal(track, m_geo, m_geo.getPlane(iref), state, chi2, dof, iref);
      event.getSensorEvent(iref).setLocalState(itrack, state);
    }
  }
}
