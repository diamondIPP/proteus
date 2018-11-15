/**
 * \file
 * \brief Common typedefs
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-09
 */

#ifndef PT_DEFINITIONS_H
#define PT_DEFINITIONS_H

#include <cassert>
#include <utility>

#include <Eigen/Cholesky>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/LU>

// Use to number and identify things, e.g. hits, sensors
using Index = unsigned int;
constexpr Index kInvalidIndex = static_cast<Index>(-1);

// Digital matrix position defined by column and row index
typedef std::pair<Index, Index> ColumnRow;

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
using Matrix23 = Matrix<Scalar, 2, 3>;
using Matrix26 = Matrix<Scalar, 2, 6>;
using Matrix3 = Matrix<Scalar, 3, 3>;
using Matrix32 = Matrix<Scalar, 3, 2>;
using Matrix4 = Matrix<Scalar, 4, 4>;
using Matrix6 = Matrix<Scalar, 6, 6>;
using DiagMatrix6 = DiagMatrix<Scalar, 6>;
using SymMatrix2 = SymMatrix<Scalar, 2>;
using SymMatrix3 = SymMatrix<Scalar, 3>;
using SymMatrix4 = SymMatrix<Scalar, 4>;
using SymMatrix6 = SymMatrix<Scalar, 6>;
using Vector2 = Vector<Scalar, 2>;
using Vector3 = Vector<Scalar, 3>;
using Vector4 = Vector<Scalar, 4>;
using Vector6 = Vector<Scalar, 6>;

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
 * The vector elements are weighted with the inverse of a covariance matrix.
 * This is a multi-dimensional generalization of the pull / significance
 * measure.
 */
template <typename T, typename U>
inline auto mahalanobisSquared(const Eigen::MatrixBase<T>& cov,
                               const Eigen::MatrixBase<U>& x)
{
  // compute `x^T C^-1 x` via `x^T y` where `y` is the solution to `C y = x`
  return x.dot(cov.template selfadjointView<Eigen::Lower>().llt().solve(x));
}

/** Mahalanobis distance / norm of a vector. */
template <typename T, typename U>
inline auto mahalanobis(const Eigen::MatrixBase<T>& cov,
                        const Eigen::MatrixBase<U>& x)
{
  return std::sqrt(mahalanobisSquared(cov, x));
}

/** Conversion from angle in radian to equivalent angle in degrees. */
static constexpr double degree(double radian) { return radian * 180.0 / M_PI; }

#endif // PT_DEFINITIONS_H
