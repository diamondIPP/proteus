// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \brief Common typedefs
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-09
 */

#pragma once

#include <utility>

#include <Eigen/Cholesky>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/LU>

namespace proteus {

// Use to number and identify things, e.g. hits, sensors
using Index = unsigned int;
constexpr Index kInvalidIndex = static_cast<Index>(-1);

// Digital matrix position defined by column and row index
using ColumnRow = std::pair<Index, Index>;

// Default floating point scalar type
using Scalar = double;

// Templated vector types
template <typename T, int kRows, int kCols>
using Matrix = Eigen::Matrix<T, kRows, kCols>;
template <typename T, int kSize>
using DiagMatrix = Eigen::DiagonalMatrix<T, kSize>;
// Eigen has no special symmetric type, keep it for annotation
template <typename T, int kSize>
using SymMatrix = Eigen::Matrix<T, kSize, kSize>;
template <typename T, int kSize>
using Vector = Eigen::Matrix<T, kSize, 1>;

// Commonly used vector and matrix types
// For non-quadratic matrices the first number is the target dimensionality and
// the second number is the source dimensionality.
using Matrix2 = Matrix<Scalar, 2, 2>;
using Matrix3 = Matrix<Scalar, 3, 3>;
using Matrix4 = Matrix<Scalar, 4, 4>;
using Matrix5 = Matrix<Scalar, 5, 5>;
using Matrix6 = Matrix<Scalar, 6, 6>;
using DiagMatrix4 = DiagMatrix<Scalar, 4>;
using DiagMatrix6 = DiagMatrix<Scalar, 6>;
using SymMatrix2 = SymMatrix<Scalar, 2>;
using SymMatrix3 = SymMatrix<Scalar, 3>;
using SymMatrix4 = SymMatrix<Scalar, 4>;
using SymMatrix6 = SymMatrix<Scalar, 6>;
using Vector2 = Vector<Scalar, 2>;
using Vector3 = Vector<Scalar, 3>;
using Vector4 = Vector<Scalar, 4>;
using Vector6 = Vector<Scalar, 6>;

// convenience enums to access coordinates or track parameters by name
enum CoordinateIndex {
  kX = 0,
  kY = 1,
  kZ = 2,
  kT = 3,
  kU = kX,
  kV = kY,
  kW = kZ,
  kS = kT,
};
enum TrackParameterIndex {
  // start of the 3-element on-plane spatial and temporal parameters block
  kOnPlane = 0,
  // on-plane single element access
  kLoc0 = kOnPlane + 0,
  kLoc1 = kOnPlane + 1,
  kTime = kOnPlane + 2,
  // start of the 2-element spatial slope parameters block
  kSlope = 3,
  // single slope access
  kSlopeLoc0 = kSlope + 0,
  kSlopeLoc1 = kSlope + 1,
  // inverse velocity
  kSlopeTime = 5,
};

/** Transform the covariance to a different base using a Jacobian.
 *
 * Compute `jac * cov * jac^T`. This only gives the correct result if
 * the input covariance is symmetric.
 */
template <typename Jacobian, typename Covariance>
inline auto transformCovariance(const Eigen::MatrixBase<Jacobian>& jac,
                                const Eigen::EigenBase<Covariance>& cov)
{
  // w/o the eval, all hell breaks loose and the compiler seems to
  // optimize things away that should stay where they are.
  return (jac * cov.derived() * jac.transpose()).eval();
}
template <typename Jacobian, typename Covariance>
inline auto transformCovariance(const Eigen::DiagonalBase<Jacobian>& jac,
                                const Eigen::EigenBase<Covariance>& cov)
{
  // see above for eval
  return (jac * cov.derived() * jac).eval();
}

/** Extract the standard deviation vector from a covariance matrix. */
template <typename Covariance>
inline auto extractStdev(const Eigen::EigenBase<Covariance>& cov)
{
  return cov.derived().diagonal().cwiseSqrt().eval();
}

/** Squared Mahalanobis distance / norm of a vector.
 *
 * \param cov Covariance matrix; only the lower-left triangular values are used.
 * \param x   Value vector;
 *
 * The vector elements are weighted with the inverse of a covariance matrix.
 * This is a multi-dimensional generalization of the pull / significance
 * measure.
 */
template <typename T, typename U>
inline auto mahalanobisSquared(const Eigen::MatrixBase<T>& cov,
                               const Eigen::MatrixBase<U>& x)
{
  // compute `x^T C^-1 x` via `x^T y` where `y` is the solution to `C y = x`
  return x.dot(cov.template selfadjointView<Eigen::Lower>().ldlt().solve(x));
}

/** Mahalanobis distance / norm of a vector. */
template <typename T, typename U>
inline auto mahalanobis(const Eigen::MatrixBase<T>& cov,
                        const Eigen::MatrixBase<U>& x)
{
  return std::sqrt(mahalanobisSquared(cov, x));
}

/** Conversion from angle in radian to equivalent angle in degrees. */
constexpr double degree(double radian) { return radian * 180.0 / M_PI; }

/** Print format helper for Eigen types. */
template <typename T>
inline Eigen::WithFormat<T> format(const Eigen::DenseBase<T>& mtx)
{
  if ((mtx.rows() == 1) or (mtx.cols() == 1)) {
    // print all vectors as row vectors [x, y, z, ...]
    return mtx.format(Eigen::IOFormat(4, 0, ",", "", "", "", "[", "]"));
  } else {
    // print matrices as
    // [ a, b, c
    //   d, e, f]
    return mtx.format(Eigen::IOFormat(4, 0, ",", "\n", "", ",", "[", "]"));
  }
}

} // namespace proteus
