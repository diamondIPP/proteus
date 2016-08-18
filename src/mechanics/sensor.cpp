#include "sensor.h"

#include <cassert>
#include <iostream>
#include <math.h>
#include <string>

#include "mechanics/device.h"
#include "mechanics/noisemask.h"

using std::cout;
using std::endl;

Mechanics::Sensor::Sensor(const std::string& name,
                          Index numCols,
                          Index numRows,
                          double pitchCol,
                          double pitchRow,
                          double depth,
                          double xX0)
    : m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_depth(depth)
    , m_xox0(xX0)
    , m_name(name)
    , m_noiseMask(numCols * numRows, false)
    , m_device(nullptr)
{
}

Mechanics::Sensor::Sensor(unsigned int numX,
                          unsigned int numY,
                          double pitchX,
                          double pitchY,
                          double depth,
                          Device* device,
                          std::string name,
                          bool digi,
                          bool alignable,
                          double xox0,
                          double offX,
                          double offY,
                          double offZ,
                          double rotX,
                          double rotY,
                          double rotZ)
    : m_numCols(numX)
    , m_numRows(numY)
    , m_pitchCol(pitchX)
    , m_pitchRow(pitchY)
    , m_depth(depth)
    , m_xox0(xox0)
    , m_name(name)
    , m_device(device)
    , m_noiseMask(numX * numY, false)
    , m_digi(digi)
    , m_alignable(alignable)
{
  assert(device && "Sensor: need to link the sensor back to a device.");
}

// geometry related methods

XYZVector Mechanics::Sensor::normal() const
{
  XYZVector W, U, V;
  m_l2g.Rotation().GetComponents(U, V, W);
  return W;
}

double Mechanics::Sensor::getOffX() const
{
  return m_l2g.Translation().Vect().x();
}

double Mechanics::Sensor::getOffY() const
{
  return m_l2g.Translation().Vect().y();
}

double Mechanics::Sensor::getOffZ() const
{
  return m_l2g.Translation().Vect().z();
}

void Mechanics::Sensor::getGlobalOrigin(double& x, double& y, double& z) const
{
  origin().GetCoordinates(x, y, z);
}

void Mechanics::Sensor::getNormalVector(double& x, double& y, double& z) const
{
  normal().GetCoordinates(x, y, z);
}

void Mechanics::Sensor::rotateToGlobal(double& x, double& y, double& z) const
{
  XYZVector uvw(x, y, z);
  XYZVector xyz = m_l2g * uvw;
  x = xyz.x();
  y = xyz.y();
  z = xyz.z();
}

void Mechanics::Sensor::rotateToSensor(double& x, double& y, double& z) const
{
  XYZVector xyz(x, y, z);
  XYZVector uvw = m_l2g.Inverse() * xyz;
  x = uvw.x();
  y = uvw.y();
  z = uvw.z();
}

void Mechanics::Sensor::pixelToSpace(
    double col, double row, double& x, double& y, double& z) const
{
  const double halfU = getSensitiveX() / 2.0;
  const double halfV = getSensitiveY() / 2.0;
  // convert digital position in pixels to local metric coordinates with its
  // origin at the center of the sensitive area.
  XYZPoint uvw(col * m_pitchCol - halfU, row * m_pitchRow - halfV, 0);
  XYZPoint xyz = m_l2g * uvw;
  x = xyz.x();
  y = xyz.y();
  z = xyz.z();
}

void Mechanics::Sensor::spaceToPixel(
    double x, double y, double z, double& col, double& row) const
{
  const double halfU = getSensitiveX() / 2.0;
  const double halfV = getSensitiveY() / 2.0;
  // global to local metric coordinates
  XYZPoint xyz(x, y, z);
  XYZPoint uvw = m_l2g.Inverse() * xyz;
  // convert local metric coordinates back to digital pixel colums and rows
  // local origin is at the center of the sensitive area
  col = (uvw.x() + halfU) / m_pitchCol;
  row = (uvw.y() + halfV) / m_pitchRow;
}

//=========================================================
//
// noisy-pixels functions
//
//=========================================================

void Mechanics::Sensor::setNoisyPixels(const std::set<ColumnRow>& pixels)
{
	// reset possible existing mask
	std::fill(m_noiseMask.begin(), m_noiseMask.end(), false);

  for (auto it = pixels.begin(); it != pixels.end(); ++it) {
    auto col = std::get<0>(*it);
    auto row = std::get<1>(*it);

    if (m_numCols <= col)
      throw std::runtime_error("column index is out of range");
    if (m_numRows <= row)
      throw std::runtime_error("row index is out of range");

		m_noiseMask[linearPixelIndex(col, row)] = true;
  }
}

//=========================================================
//
// Misc functions
//
//=========================================================
void Mechanics::Sensor::print()
{
  cout << "\nSENSOR:\n"
       << "  Name: '" << m_name << "'\n"
       << "  Transform: " << m_l2g << '\n'
       //  << "  Pos: " << m_offX << " , " << m_offY << " , " << m_offZ << "\n"
       //  << "  Rot: " << m_rotX << " , " << m_rotY << " , " << m_rotZ << "\n"
       << "  Cols: " << getNumX() << "\n"
       << "  Rows: " << getNumY() << "\n"
       << "  Pitch-x: " << m_pitchCol << "\n"
       << "  Pitch-y: " << m_pitchRow << "\n"
       << "  Depth: " << m_depth << "\n"
       << "  X / X0: " << m_xox0 << "\n"
       << "  Sensitive X: " << getSensitiveX() << "\n"
       << "  Sensitive Y: " << getSensitiveY() << "\n"
       << "  PosNumX: " << getPosNumX() << "\n"
       << "  PosNumY: " << getPosNumY() << "\n"
       << "  Alignable: " << m_alignable << endl;

  // TODO 2016-08-18 msmk: how to print?
  // cout << "  Noisy pixels (" << m_numNoisyPixels << ")" << endl;
  // if (m_numNoisyPixels < 20) {
  //   for (unsigned int nx = 0; nx < getNumX(); nx++)
  //     for (unsigned int ny = 0; ny < getNumY(); ny++)
  //       if (m_noisyPixels[nx][ny])
  //         cout << "    " << nx << " : " << ny << endl;
  // }
}

bool Mechanics::Sensor::sort(const Sensor* s1, const Sensor* s2)
{
  return (s1->getOffZ() < s2->getOffZ());
}

//=========================================================
//
// get functions
//
//=========================================================

unsigned int Mechanics::Sensor::getPosNumX() const
{
  return (unsigned int)((getPosSensitiveX() / getPosPitchX()) + 0.5);
}

unsigned int Mechanics::Sensor::getPosNumY() const
{
  return (unsigned int)((getPosSensitiveY() / getPosPitchY()) + 0.5);
}

double Mechanics::Sensor::getPosPitchX() const
{
  double pitch = 0;
  double x, y, z;
  // Make a vector pointing along the pixel's x-axis
  x = getPitchX();
  y = 0;
  z = 0;
  // Get it's x-projection
  rotateToGlobal(x, y, z);
  pitch += fabs(x);
  // Again with the pixel's y-axis
  x = 0;
  y = getPitchY();
  z = 0;
  rotateToGlobal(x, y, z);
  pitch += fabs(x);
  // Return the sum of the x-projection
  return pitch;
}

double Mechanics::Sensor::getPosPitchY() const
{
  double pitch = 0;
  double x, y, z;
  x = getPitchX();
  y = 0;
  z = 0;
  rotateToGlobal(x, y, z);
  pitch += fabs(y);
  x = 0;
  y = getPitchY();
  z = 0;
  rotateToGlobal(x, y, z);
  pitch += fabs(y);
  return pitch;
}

double Mechanics::Sensor::getPosSensitiveX() const
{
  double size = 0;
  double x, y, z;
  x = getSensitiveX();
  y = 0;
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(x);

  x = 0;
  y = getSensitiveY();
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(x);

  return size;
}

double Mechanics::Sensor::getPosSensitiveY() const
{
  double size = 0;
  double x, y, z;
  x = getSensitiveX();
  y = 0;
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(y);
  x = 0;
  y = getSensitiveY();
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(y);
  return size;
}

bool Mechanics::Sensor::getDigital() const { return m_digi; }
bool Mechanics::Sensor::getAlignable() const { return m_alignable; }
unsigned int Mechanics::Sensor::getNumX() const { return m_numCols; }
unsigned int Mechanics::Sensor::getNumY() const { return m_numRows; }
unsigned int Mechanics::Sensor::getNumPixels() const
{
  return m_numCols * m_numRows;
}
double Mechanics::Sensor::getPitchX() const { return m_pitchCol; }
double Mechanics::Sensor::getPitchY() const { return m_pitchRow; }
double Mechanics::Sensor::getDepth() const { return m_depth; }
double Mechanics::Sensor::getXox0() const { return m_xox0; }
double Mechanics::Sensor::getSensitiveX() const
{
  return m_numCols * m_pitchCol;
}
double Mechanics::Sensor::getSensitiveY() const
{
  return m_numRows * m_pitchRow;
}
const Mechanics::Device* Mechanics::Sensor::getDevice() const
{
  return m_device;
}
const char* Mechanics::Sensor::getName() const { return m_name.c_str(); }
