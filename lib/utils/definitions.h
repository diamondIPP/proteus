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

#include <Math/SMatrix.h>
#include <Math/SVector.h>

// Use to number and identify things, e.g. hits, sensors
typedef unsigned int Index;
constexpr Index kInvalidIndex = static_cast<Index>(-1);

// Digital matrix position defined by column and row index
typedef std::pair<Index, Index> ColumnRow;

// Templated vector types
template <typename T, unsigned int kRows, unsigned int kCols>
using Matrix = ROOT::Math::SMatrix<T, kRows, kCols>;
template <typename T, unsigned int kSize>
using SymMatrix =
    ROOT::Math::SMatrix<T, kSize, kSize, ROOT::Math::MatRepSym<T, kSize>>;
template <typename T, unsigned int kSize>
using Vector = ROOT::Math::SVector<T, kSize>;

// Commonly used vector and matrix types
// For non-quadratic matrices the first number is the target dimensionality and
// the second number is the source dimensionality.
using Matrix2 = Matrix<double, 2, 2>;
using Matrix23 = Matrix<double, 2, 3>;
using Matrix26 = Matrix<double, 2, 6>;
using Matrix3 = Matrix<double, 3, 3>;
using Matrix32 = Matrix<double, 3, 2>;
using Matrix4 = Matrix<double, 4, 4>;
using Matrix6 = Matrix<double, 6, 6>;
using SymMatrix2 = SymMatrix<double, 2>;
using SymMatrix3 = SymMatrix<double, 3>;
using SymMatrix4 = SymMatrix<double, 4>;
using SymMatrix6 = SymMatrix<double, 6>;
using Vector2 = Vector<double, 2>;
using Vector3 = Vector<double, 3>;
using Vector4 = Vector<double, 4>;
using Vector6 = Vector<double, 6>;

/** Squared Mahalanobis distance / norm of a vector.
 *
 * The vector elements are weighted with the inverse of a covariance matrix.
 * This is a multi-dimensional generalization of the pull / significance
 * measure.
 */
template <typename T, unsigned int kD1>
static inline double mahalanobisSquared(
    const ROOT::Math::SMatrix<T, kD1, kD1, ROOT::Math::MatRepSym<T, kD1>>& cov,
    const ROOT::Math::SVector<T, kD1>& x)
{
  ROOT::Math::SMatrix<T, kD1, kD1, ROOT::Math::MatRepSym<T, kD1>> weight(cov);
  if (!weight.InvertChol())
    throw std::runtime_error(
        "Covariance inversion failed for Mahalanobis distance");
  return ROOT::Math::Dot(x, weight * x);
}

/** Mahalanobis distance / norm of a vector. */
template <typename T, unsigned int kD1>
static inline double mahalanobis(
    const ROOT::Math::SMatrix<T, kD1, kD1, ROOT::Math::MatRepSym<T, kD1>>& cov,
    const ROOT::Math::SVector<T, kD1>& x)
{
  return std::sqrt(mahalanobisSquared(cov, x));
}

#endif // PT_DEFINITIONS_H
