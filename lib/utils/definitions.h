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

// commonly used vector and matrix types
typedef ROOT::Math::SMatrix<double, 2, 2> Matrix2;
typedef ROOT::Math::SMatrix<double, 2, 3> Matrix23;
typedef ROOT::Math::SMatrix<double, 2, 6> Matrix26;
typedef ROOT::Math::SMatrix<double, 3, 3> Matrix3;
typedef ROOT::Math::SMatrix<double, 4, 4> Matrix4;
typedef ROOT::Math::SMatrix<double, 3, 2> Matrix32;
typedef ROOT::Math::SMatrix<double, 6, 6> Matrix6;
typedef ROOT::Math::SMatrix<double, 2, 2, ROOT::Math::MatRepSym<double, 2>>
    SymMatrix2;
typedef ROOT::Math::SMatrix<double, 3, 3, ROOT::Math::MatRepSym<double, 3>>
    SymMatrix3;
typedef ROOT::Math::SMatrix<double, 4, 4, ROOT::Math::MatRepSym<double, 4>>
    SymMatrix4;
typedef ROOT::Math::SMatrix<double, 6, 6, ROOT::Math::MatRepSym<double, 6>>
    SymMatrix6;
typedef ROOT::Math::SVector<double, 2> Vector2;
typedef ROOT::Math::SVector<double, 3> Vector3;
typedef ROOT::Math::SVector<double, 4> Vector4;
typedef ROOT::Math::SVector<double, 6> Vector6;

/** Squared Mahalanobis distance / norm of a vector.
 *
 * The vector elements are weighted with the inverse of a covariance matrix.
 * This is a multi-dimensional generalization of the pull / significance
 * measure.
 */
template <typename T, unsigned int kD1>
inline double mahalanobisSquared(
    const ROOT::Math::SMatrix<T, kD1, kD1, ROOT::Math::MatRepSym<T, kD1>>& cov,
    const ROOT::Math::SVector<T, kD1>& x)
{
  ROOT::Math::SMatrix<T, kD1, kD1, ROOT::Math::MatRepSym<T, kD1>> weight(cov);
  if (!weight.InvertChol())
    throw std::runtime_error(
        "Covariance inversion failed for Mahalanobis distance");
  return ROOT::Math::Similarity(weight, x);
}

/** Mahalanobis distance / norm of a vector. */
template <typename T, unsigned int kD1>
inline double mahalanobis(
    const ROOT::Math::SMatrix<T, kD1, kD1, ROOT::Math::MatRepSym<T, kD1>>& cov,
    const ROOT::Math::SVector<T, kD1>& x)
{
  return std::sqrt(mahalanobisSquared(cov, x));
}

#endif // PT_DEFINITIONS_H
