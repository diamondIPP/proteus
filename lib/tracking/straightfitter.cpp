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
executeImpl(const Geometry& geo, const bool fitUnbiased, Event& event)
{
  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    Track& track = event.getTrack(itrack);

    // global fit for common goodness-of-fit and common global parameters
    {
      Fitter fitter;
      // add all clusters in the global system
      for (const auto& c : track.clusters()) {
        const Plane& source = geo.getPlane(c.first);
        const Cluster& cluster = c.second;
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
        if (fitUnbiased and (c.first == iref)) {
          continue;
        }
        const Plane& source = geo.getPlane(c.first);
        const Cluster& cluster = c.second;
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

Tracking::StraightFitter::StraightFitter(const Device& device)
    : m_geo(device.geometry())
{
}

std::string Tracking::StraightFitter::name() const { return "StraightFitter"; }

void Tracking::StraightFitter::execute(Event& event) const
{
  executeImpl<LineFitter3D>(m_geo, false, event);
}

Tracking::UnbiasedStraightFitter::UnbiasedStraightFitter(const Device& device)
    : m_geo(device.geometry())
{
}

std::string Tracking::UnbiasedStraightFitter::name() const
{
  return "UnbiasedStraightFitter";
}

void Tracking::UnbiasedStraightFitter::execute(Event& event) const
{
  executeImpl<LineFitter3D>(m_geo, true, event);
}
