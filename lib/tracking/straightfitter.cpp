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

using namespace Mechanics;
using namespace Storage;

template <typename Fitter>
static inline void
executeImpl(const Geometry& geo, bool fitUnbiased, Event& event)
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit and common global parameters
    {
      Fitter fitter;
      // add all clusters in the global system
      for (const auto& c : track.clusters()) {
        const Plane& source = geo.getPlane(c.sensor);
        const Cluster& cluster =
            event.getSensorEvent(c.sensor).getCluster(c.cluster);
        Vector4 global = source.toGlobal(cluster.position());
        Vector4 weight =
            transformCovariance(source.linearToGlobal(), cluster.positionCov())
                .diagonal()
                .cwiseInverse();
        fitter.addPoint(global, weight);
      }
      fitter.fit();
      track.setGlobalState(fitter.params(), fitter.cov());
      track.setGoodnessOfFit(fitter.chi2(), fitter.dof());
    }

    // local fit for optimal parameters/covariance on each sensor plane
    for (Index iref = 0; iref < event.numSensorEvents(); ++iref) {
      Fitter fitter;
      // add all clusters in the target local system
      const Plane& target = geo.getPlane(iref);
      for (const auto& c : track.clusters()) {
        // exclude measurements on target plane for unbiased fit
        if (fitUnbiased and (c.sensor == iref)) {
          continue;
        }
        const Plane& source = geo.getPlane(c.sensor);
        const Cluster& cluster =
            event.getSensorEvent(c.sensor).getCluster(c.cluster);
        Vector4 local = target.toLocal(source.toGlobal(cluster.position()));
        Matrix4 jac = target.linearToLocal() * source.linearToGlobal();
        Vector4 weight = transformCovariance(jac, cluster.positionCov())
                             .diagonal()
                             .cwiseInverse();
        fitter.addPoint(local, weight);
      }
      fitter.fit();
      // local fits only update the local state; not the global fit quality
      event.getSensorEvent(iref).setLocalState(itrack, fitter.params(),
                                               fitter.cov());
    }
  }
}
// straight 3d

Tracking::Straight3dFitter::Straight3dFitter(const Device& device)
    : m_geo(device.geometry())
{
}

std::string Tracking::Straight3dFitter::name() const
{
  return "Straight3dFitter";
}

void Tracking::Straight3dFitter::execute(Event& event) const
{
  executeImpl<LineFitter3D>(m_geo, false, event);
}

// straight 4d

Tracking::Straight4dFitter::Straight4dFitter(const Device& device)
    : m_geo(device.geometry())
{
}

std::string Tracking::Straight4dFitter::name() const
{
  return "Straight4dFitter";
}

void Tracking::Straight4dFitter::execute(Event& event) const
{
  executeImpl<LineFitter4D>(m_geo, false, event);
}

// unbiased straight 3d

Tracking::UnbiasedStraight3dFitter::UnbiasedStraight3dFitter(
    const Device& device)
    : m_geo(device.geometry())
{
}

std::string Tracking::UnbiasedStraight3dFitter::name() const
{
  return "UnbiasedStraight3dFitter";
}

void Tracking::UnbiasedStraight3dFitter::execute(Event& event) const
{
  executeImpl<LineFitter3D>(m_geo, true, event);
}

// unbiased straight 4d

Tracking::UnbiasedStraight4dFitter::UnbiasedStraight4dFitter(
    const Device& device)
    : m_geo(device.geometry())
{
}

std::string Tracking::UnbiasedStraight4dFitter::name() const
{
  return "UnbiasedStraight4dFitter";
}

void Tracking::UnbiasedStraight4dFitter::execute(Event& event) const
{
  executeImpl<LineFitter4D>(m_geo, true, event);
}
