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

#include <Math/GenVector/Rotation3D.h>
#include <Math/GenVector/RotationZYX.h>
#include <Math/GenVector/Transform3D.h>
#include <Math/GenVector/Translation3D.h>
#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/SMatrix.h>
#include <Math/SVector.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

typedef unsigned int Index;
// Digital matrix position defined by column and row index
typedef std::pair<Index, Index> ColumnRow;

// commonly used geometry types
typedef ROOT::Math::XYPoint XYPoint;
typedef ROOT::Math::XYZPoint XYZPoint;
typedef ROOT::Math::XYVector XYVector;
typedef ROOT::Math::XYZVector XYZVector;
typedef ROOT::Math::Translation3D Translation3D;
typedef ROOT::Math::Rotation3D Rotation3D;
typedef ROOT::Math::RotationZYX RotationZYX;
typedef ROOT::Math::Transform3D Transform3D;

// commonly used vector and matrix types
typedef ROOT::Math::SMatrix<double, 2, 2> Matrix2;
typedef ROOT::Math::SMatrix<double, 3, 3> Matrix3;
typedef ROOT::Math::SMatrix<double, 4, 4> Matrix4;
typedef ROOT::Math::SMatrix<double, 3, 2> Matrix32;
typedef ROOT::Math::SMatrix<double, 3, 4> Matrix34;
typedef ROOT::Math::SMatrix<double, 2, 2, ROOT::Math::MatRepSym<double, 2>>
    SymMatrix2;
typedef ROOT::Math::SMatrix<double, 3, 3, ROOT::Math::MatRepSym<double, 3>>
    SymMatrix3;
typedef ROOT::Math::SMatrix<double, 4, 4, ROOT::Math::MatRepSym<double, 4>>
    SymMatrix4;
typedef ROOT::Math::SVector<double, 2> Vector2;
typedef ROOT::Math::SVector<double, 3> Vector3;
typedef ROOT::Math::SVector<double, 4> Vector4;

/** Squared Mahalanobis distance / norm of a vector.
 *
 * The vector elements are weighted with the inverse of a covariance matrix.
 * This is a multi-dimensional generalization of the pull / significance
 * measure. In the case of a Gaussian distribution this is also equivalent
 * to chi2.
 */
template <typename T, unsigned int D1>
inline double mahalanobisSquared(
    const ROOT::Math::SMatrix<T, D1, D1, ROOT::Math::MatRepSym<T, D1>>& cov,
    const ROOT::Math::SVector<T, D1>& x)
{
  ROOT::Math::SMatrix<T, D1, D1, ROOT::Math::MatRepSym<T, D1>> weight(cov);
  int fail;
  weight.InverseChol(fail);
  assert(!fail && "Covariance inversion failed");
  return ROOT::Math::Similarity(weight, x);
}

/** Mahalanobis distance / norm of a vector. */
template <typename T, unsigned int D1>
inline double mahalanobis(
    const ROOT::Math::SMatrix<T, D1, D1, ROOT::Math::MatRepSym<T, D1>>& cov,
    const ROOT::Math::SVector<T, D1>& x)
{
  return std::sqrt(mahalanobisSquared(cov, x));
}

#endif // PT_DEFINITIONS_H
