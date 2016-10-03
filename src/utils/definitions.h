#ifndef PT_DEFINITIONS_H
#define PT_DEFINITIONS_H

#include <utility>

#include <Math/GenVector/Rotation3D.h>
#include <Math/GenVector/RotationZYX.h>
#include <Math/GenVector/Transform3D.h>
#include <Math/GenVector/Translation3D.h>
#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

using Index = unsigned int;
// Digital matrix position defined by column and row index
using ColumnRow = std::pair<Index, Index>;

// commonly used vector and geometry types
using XYPoint = ROOT::Math::XYPoint;
using XYZPoint = ROOT::Math::XYZPoint;
using XYVector = ROOT::Math::XYVector;
using XYZVector = ROOT::Math::XYZVector;
using Translation3D = ROOT::Math::Translation3D;
using Rotation3D = ROOT::Math::Rotation3D;
using RotationZYX = ROOT::Math::RotationZYX;
using Transform3D = ROOT::Math::Transform3D;

#endif // PT_DEFINITIONS_H
