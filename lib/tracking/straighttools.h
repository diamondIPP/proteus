/**
 * \file
 * \brief Common tools for straight track fitting
 * \author Moritz Kiehn (msmk@cern.ch)
 */

#ifndef PT_STRAIGHTTOOLS_H
#define PT_STRAIGHTTOOLS_H

#include "mechanics/geometry.h"
#include "storage/cluster.h"
#include "storage/track.h"
#include "utils/definitions.h"

namespace Tracking {

/** Fit a line in two dimensions using a linear weighted regression.
 *
 * Straight from Numerical Recipes with offset = a and slope = b.
 */
struct LineFitter2D {
  double s, sx, sy, sxx, sxy, syy; // weighted sums of inputs
  double cxx;                      // (unscaled) input variance

  LineFitter2D() : s(0), sx(0), sy(0), sxx(0), sxy(0), syy(0), cxx(0) {}

  void addPoint(double x, double y, double w = 1);
  void fit();

  double offset() const { return (sy * sxx - sx * sxy) / cxx; }
  double slope() const { return (s * sxy - sx * sy) / cxx; }
  double varOffset() const { return sxx / cxx; }
  double varSlope() const { return s / cxx; }
  double cov() const { return -sx / cxx; }
  double chi2() const;
};

/** Fit a line in three dimensions as a function of the third dimension.
 *
 * Assumes uncorrelated uncertainties both between the two transverse
 * dimensions and between different points.
 */
struct LineFitter3D {
  LineFitter2D lineU, lineV;

  LineFitter3D() = default;

  void addPoint(double u, double v, double w, double wu = 1, double wv = 1);
  /** Add a cluster as seen in the global coordinates. */
  void addPoint(const Storage::Cluster& cluster,
                const Mechanics::Plane& source);
  /** Add a cluster as seen in the target local coordinates. */
  void addPoint(const Storage::Cluster& cluster,
                const Mechanics::Plane& source,
                const Mechanics::Plane& target);
  void fit();

  Storage::TrackState state() const;
  double chi2() const { return lineU.chi2() + lineV.chi2(); }
};

/** Fit a straight track in the global coordinates.
 *
 * \param[in] geo The setup geometry with the local-global transformations
 * \param[in,out] track The global track state is set to fit result.
 */
void fitStraightTrackGlobal(const Mechanics::Geometry& geo,
                            Storage::Track& track);

} // namespace Tracking

inline void Tracking::LineFitter2D::addPoint(double x, double y, double w)
{
  s += w;
  sx += w * x;
  sy += w * y;
  sxx += w * x * x;
  sxy += w * x * y;
  syy += w * y * y;
}

inline void Tracking::LineFitter2D::fit() { cxx = (s * sxx - sx * sx); }

inline double Tracking::LineFitter2D::chi2() const
{
  return syy + (sxy * (2 * sx * sy - s * sxy) - sxx * sy * sy) / cxx;
}

inline void Tracking::LineFitter3D::addPoint(
    double u, double v, double w, double wu, double wv)
{
  lineU.addPoint(w, u, wu);
  lineV.addPoint(w, v, wv);
}

inline void Tracking::LineFitter3D::addPoint(const Storage::Cluster& cluster,
                                             const Mechanics::Plane& source)
{
  auto rot = source.rotation.Sub<Matrix32>(0, 0);
  // local to global assuming a position on the local plane
  Vector2 abc(cluster.posLocal().x(), cluster.posLocal().y());
  Vector3 xyz = source.offset + rot * abc;
  Matrix3 cov = Similarity(rot, cluster.covLocal());
  addPoint(xyz[0], xyz[1], xyz[2], 1 / cov(0, 0), 1 / cov(1, 1));
}

inline void Tracking::LineFitter3D::addPoint(const Storage::Cluster& cluster,
                                             const Mechanics::Plane& source,
                                             const Mechanics::Plane& target)
{
  auto rot = source.rotation.Sub<Matrix32>(0, 0);
  // source local to global assuming a position on the local plane
  Vector2 abc(cluster.posLocal().x(), cluster.posLocal().y());
  Vector3 xyz = source.offset + rot * abc;
  // global to target local
  Vector3 uvw = Transpose(target.rotation) * (xyz - target.offset);
  Matrix3 cov =
      Similarity(Transpose(target.rotation) * rot, cluster.covLocal());
  addPoint(uvw[0], uvw[1], uvw[2], 1 / cov(0, 0), 1 / cov(1, 1));
}

inline void Tracking::LineFitter3D::fit()
{
  lineU.fit();
  lineV.fit();
}

inline Storage::TrackState Tracking::LineFitter3D::state() const
{
  Storage::TrackState s(lineU.offset(), lineV.offset(), lineU.slope(),
                        lineV.slope());
  s.setCovU(lineU.varOffset(), lineU.varSlope(), lineU.cov());
  s.setCovV(lineV.varOffset(), lineV.varSlope(), lineV.cov());
  return s;
}

inline void Tracking::fitStraightTrackGlobal(const Mechanics::Geometry& geo,
                                             Storage::Track& track)
{
  LineFitter3D fitter;

  for (const auto& c : track.clusters())
    fitter.addPoint(c.second, geo.getPlane(c.first));
  fitter.fit();
  track.setGlobalState(fitter.state());
  track.setGoodnessOfFit(fitter.chi2(), 2 * (track.size() - 2));
}

#endif // PT_STRAIGHTTOOLS_H