#ifndef __JUDITH__DEFINITIONS_H__
#define __JUDITH__DEFINITIONS_H__

#include <Math/GenVector/Rotation3D.h>
#include <Math/GenVector/RotationZYX.h>
#include <Math/GenVector/Transform3D.h>
#include <Math/GenVector/Translation3D.h>
#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

// commonly used vector and geometry types
using Point2 = ROOT::Math::XYPoint;
using Point3 = ROOT::Math::XYZPoint;
using Vector2 = ROOT::Math::XYVector;
using Vector3 = ROOT::Math::XYZVector;
using Translation3 = ROOT::Math::Translation3D;
using Rotation3 = ROOT::Math::Rotation3D;
using RotationZYX = ROOT::Math::RotationZYX;
using Transform3 = ROOT::Math::Transform3D;

#endif /* end of include guard: __JUDITH__DEFINITIONS_H__ */
