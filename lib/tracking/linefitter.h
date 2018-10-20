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
  // weighted sums of inputs
  double s = 0.0;
  double sx = 0.0;
  double sy = 0.0;
  double sxx = 0.0;
  double sxy = 0.0;
  double syy = 0.0;
  // (unscaled) input variance
  double cxx = 0.0;

  void addPoint(double x, double y, double weight = 1.0)
  {
    s += weight;
    sx += weight * x;
    sy += weight * y;
    sxx += weight * x * x;
    sxy += weight * x * y;
    syy += weight * y * y;
  }
  void fit() { cxx = (s * sxx - sx * sx); }

  double offset() const { return (sy * sxx - sx * sxy) / cxx; }
  double slope() const { return (s * sxy - sx * sy) / cxx; }
  double varOffset() const { return sxx / cxx; }
  double varSlope() const { return s / cxx; }
  double cov() const { return -sx / cxx; }
  double chi2() const
  {
    return syy + (sxy * (2 * sx * sy - s * sxy) - sxx * sy * sy) / cxx;
  }
};

/** Fit a line in three dimensions as a function of the third dimension.
 *
 * Assumes uncorrelated uncertainties both between the two transverse
 * dimensions and between different points.
 */
struct LineFitter3D {
  LineFitter2D lineZX;
  LineFitter2D lineZY;
  int numPoints = 0;

  void addPoint(
      double x, double y, double z, double weightX = 1, double weightY = 1);
  /** Add a cluster as seen in the global coordinates. */
  void addPoint(const Storage::Cluster& cluster,
                const Mechanics::Plane& source);
  /** Add a cluster as seen in the target local coordinates. */
  void addPoint(const Storage::Cluster& cluster,
                const Mechanics::Plane& source,
                const Mechanics::Plane& target);
  void fit();

  /** Fitted parameters [x, y, slopeX, slopeY]. */
  Vector4 params() const;
  /** Fitted parameter covariance [x, y, slopeX, slopeY]. */
  SymMatrix4 cov() const;
  Storage::TrackState state() const;
  double chi2() const { return lineZX.chi2() + lineZY.chi2(); }
  int dof() const { return 2 * numPoints - 4; }
};

/** Fit a straight track in the global coordinates.
 *
 * \param[in] geo The setup geometry with the local-global transformations
 * \param[in,out] track The global track state is set to fit result.
 */
void fitStraightTrackGlobal(const Mechanics::Geometry& geo,
                            Storage::Track& track);

} // namespace Tracking

inline void Tracking::LineFitter3D::addPoint(
    double x, double y, double z, double weightX, double weightY)
{
  lineZX.addPoint(z, x, weightX);
  lineZY.addPoint(z, y, weightY);
  numPoints += 1;
}

inline void Tracking::LineFitter3D::fit()
{
  lineZX.fit();
  lineZY.fit();
}

inline Vector4 Tracking::LineFitter3D::params() const
{
  return {lineZX.offset(), lineZY.offset(), lineZX.slope(), lineZY.slope()};
}

inline SymMatrix4 Tracking::LineFitter3D::cov() const
{
  SymMatrix4 cov = SymMatrix4::Zero();
  cov(0, 0) = lineZX.varOffset();
  cov(1, 1) = lineZY.varOffset();
  cov(2, 2) = lineZX.varSlope();
  cov(3, 3) = lineZY.varSlope();
  cov(0, 2) = cov(2, 0) = lineZX.cov();
  cov(1, 3) = cov(3, 1) = lineZY.cov();
  return cov;
}

inline Storage::TrackState Tracking::LineFitter3D::state() const
{
  Storage::TrackState s(params());
  s.setCov(cov());
  return s;
}

inline void Tracking::LineFitter3D::addPoint(const Storage::Cluster& cluster,
                                             const Mechanics::Plane& source)
{
  // local to global assuming a position on the local plane
  Vector3 xyz = source.toGlobal(cluster.posLocal());
  SymMatrix3 cov = transformCovariance(source.rotationToGlobal().leftCols<2>(),
                                       cluster.covLocal());
  addPoint(xyz[0], xyz[1], xyz[2], 1 / cov(0, 0), 1 / cov(1, 1));
}

inline void Tracking::LineFitter3D::addPoint(const Storage::Cluster& cluster,
                                             const Mechanics::Plane& source,
                                             const Mechanics::Plane& target)
{
  // source local to target local assuming a position on the local source plane
  Vector3 uvw = target.toLocal(source.toGlobal(cluster.posLocal()));
  // the extra eval is needed to make older Eigen/compiler versions happy
  Matrix2 jac = target.rotationToLocal().eval().topRows<2>() *
                source.rotationToGlobal().leftCols<2>();
  SymMatrix2 cov = transformCovariance(jac, cluster.covLocal());
  addPoint(uvw[0], uvw[1], uvw[2], 1 / cov(0, 0), 1 / cov(1, 1));
}

inline void Tracking::fitStraightTrackGlobal(const Mechanics::Geometry& geo,
                                             Storage::Track& track)
{
  LineFitter3D fitter;

  for (const auto& c : track.clusters()) {
    fitter.addPoint(c.second, geo.getPlane(c.first));
  }
  fitter.fit();
  track.setGlobalState(fitter.state());
  track.setGoodnessOfFit(fitter.chi2(), fitter.dof());
}

#endif // PT_STRAIGHTTOOLS_H
