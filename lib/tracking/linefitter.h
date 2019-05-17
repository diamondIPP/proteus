/**
 * \file
 * \brief Tools for simple line fits in two and three dimensions
 * \author Moritz Kiehn (msmk@cern.ch)
 */

#ifndef PT_LINEFITTER_H
#define PT_LINEFITTER_H

#include <array>
#include <utility>

#include "utils/definitions.h"

namespace proteus {

/** Fit a line using linear weighted regression.
 *
 * Straight from Numerical Recipes with offset = a and slope = b.
 */
struct LineFitter {
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

/** Fit a line in multiple dimensions as a function of the I-th coordinate.
 *
 * \tparam I  Index of the independent coordinate
 * \tparam Ds Indices of the dependent coordinates
 *
 * Assumes uncorrelated uncertainties both between the dependent dimensions
 * and between different input points.
 */
template <size_t I, size_t... Ds>
struct LineFitterND {
  enum {
    kIndependent = I,
    kNDependents = sizeof...(Ds),
    kNParameters = 2 * sizeof...(Ds),
  };

  std::array<LineFitter, kNDependents> lines;
  int numPoints = 0;

  /** Add a point to the fitter.
   *
   * \param point  N-dimensional point
   * \param weight N-dimensional weight, the I-th coordinate is unused
   */
  template <typename Point, typename Weight>
  void addPoint(const Eigen::MatrixBase<Point>& point,
                const Eigen::MatrixBase<Weight>& weight);
  /** Fit the lines from all previously added points. */
  void fit();

  /** Fitted sum of squared, weighted residuals. */
  double chi2() const;
  /** Fit degrees-of-freedom. */
  int dof() const { return kNDependents * numPoints - kNParameters; }
  /** Get fit parameters.
   *
   * \tparam Os Indices that define output ordering of fitted parameters
   *
   * Internally, the parameters are ordered by coordinates, i.e.
   * [offset0, slope0, offset1, slope1, ...]. Output indices map the internal
   * ordering to the output ordering such that the i-th internal parameter
   * will be located in the indices[i]-th position. If the number of output
   * entries is larger than the available fit parameters, the extra output
   * values will be zeroed.
   */
  template <size_t... Os>
  Vector<double, sizeof...(Os)> getParams(std::index_sequence<Os...>) const;
  /** Get fit parameter covariance.
   *
   * \see getParams for details on indices
   */
  template <size_t... Os>
  SymMatrix<double, sizeof...(Os)> getCov(std::index_sequence<Os...>) const;
};

/** Fit lines in x,y as a function of z. */
struct LineFitter3D : LineFitterND<kZ, kX, kY> {
  using OutputIndices = std::
      index_sequence<kLoc0, kSlopeLoc0, kLoc1, kSlopeLoc1, kTime, kSlopeTime>;

  /** Fitted track parameters. */
  Vector6 params() const { return getParams(OutputIndices{}); }
  /** Fitted track parameter covariance. */
  SymMatrix6 cov() const { return getCov(OutputIndices{}); }
};

/** Fit lines in x,y,t as a function of z. */
struct LineFitter4D : LineFitterND<kZ, kX, kY, kT> {
  using OutputIndices = std::
      index_sequence<kLoc0, kSlopeLoc0, kLoc1, kSlopeLoc1, kTime, kSlopeTime>;

  /** Fitted track parameters. */
  Vector6 params() const { return getParams(OutputIndices{}); }
  /** Fitted track parameter covariance. */
  SymMatrix6 cov() const { return getCov(OutputIndices{}); }
};

// inline implementations

template <size_t I, size_t... Ds>
template <typename Point, typename Weight>
inline void
LineFitterND<I, Ds...>::addPoint(const Eigen::MatrixBase<Point>& point,
                                 const Eigen::MatrixBase<Weight>& weight)
{
  size_t j = 0;
  for (auto d : std::array<size_t, kNDependents>{Ds...}) {
    lines[j++].addPoint(point[kIndependent], point[d], weight[d]);
  }
  numPoints += 1;
}

template <size_t I, size_t... Ds>
inline void LineFitterND<I, Ds...>::fit()
{
  for (auto& line : lines) {
    line.fit();
  }
}

template <size_t I, size_t... Ds>
inline double LineFitterND<I, Ds...>::chi2() const
{
  double ret = 0.0;
  for (const auto& line : lines) {
    ret += line.chi2();
  }
  return ret;
}

template <size_t I, size_t... Ds>
template <size_t... Os>
inline Vector<double, sizeof...(Os)>
LineFitterND<I, Ds...>::getParams(std::index_sequence<Os...>) const
{
  static_assert(2 * sizeof...(Ds) <= sizeof...(Os), "Output too small");

  using Output = Vector<double, sizeof...(Os)>;
  using Indices = std::array<size_t, sizeof...(Os)>;

  Output out = Output::Zero();
  // map interal order [offset0, slope0, offset1, slope1, ...] to output
  Indices idx = {Os...};
  for (size_t i = 0; i < kNDependents; ++i) {
    out[idx[2 * i + 0]] = lines[i].offset();
    out[idx[2 * i + 1]] = lines[i].slope();
  }
  return out;
}

template <size_t I, size_t... Ds>
template <size_t... Os>
inline SymMatrix<double, sizeof...(Os)>
LineFitterND<I, Ds...>::getCov(std::index_sequence<Os...>) const
{
  static_assert(2 * sizeof...(Ds) <= sizeof...(Os), "Output too small");

  using Indices = std::array<size_t, sizeof...(Os)>;
  using Output = SymMatrix<double, sizeof...(Os)>;

  Output out = Output::Zero();
  // map interal order [offset0, slope0, offset1, slope1, ...] to output
  Indices idx = {Os...};
  for (size_t i = 0; i < kNDependents; ++i) {
    // base index within internal ordering
    auto ioff = idx[2 * i + 0];
    auto islp = idx[2 * i + 1];
    out(ioff, ioff) = lines[i].varOffset();
    out(islp, islp) = lines[i].varSlope();
    out(ioff, islp) = out(islp, ioff) = lines[i].cov();
  }
  return out;
}

} // namespace proteus

#endif // PT_LINEFITTER_H
