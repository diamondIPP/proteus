/**
 * \file
 * \brief Tools for simple line fits in two and three dimensions
 * \author Moritz Kiehn (msmk@cern.ch)
 */

#ifndef PT_LINEFITTER_H
#define PT_LINEFITTER_H

#include "mechanics/geometry.h"
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
  void fit();

  /** Fitted parameters [x, y, slopeX, slopeY]. */
  Vector4 params() const;
  /** Fitted parameter covariance [x, y, slopeX, slopeY]. */
  SymMatrix4 cov() const;
  /** Fitted sum of squared residuals. */
  double chi2() const { return lineZX.chi2() + lineZY.chi2(); }
  /** Fit degrees-of-freedom. */
  int dof() const { return 2 * numPoints - 4; }
};

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

#endif // PT_LINEFITTER_H
