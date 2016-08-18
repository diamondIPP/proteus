#include "sensor.h"

#include <cassert>
#include <iostream>
#include <math.h>
#include <string>

#include "mechanics/device.h"
#include "mechanics/noisemask.h"

using std::cout;
using std::endl;

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
    : m_numX(numX)
    , m_numY(numY)
    , m_pitchX(pitchX)
    , m_pitchY(pitchY)
    , m_depth(depth)
    , m_device(device)
    , m_name(name)
    , m_digi(digi)
    , m_xox0(xox0)
    , m_alignable(alignable)
    , m_sensitiveX(pitchX * numX)
    , m_sensitiveY(pitchY * numY)
    , m_numNoisyPixels(0)
{
  assert(device && "Sensor: need to link the sensor back to a device.");

  m_noisyPixels = new bool*[m_numX];
  for (unsigned int x = 0; x < m_numX; x++) {
    bool* row = new bool[m_numY];
    m_noisyPixels[x] = row;
  }
  clearNoisyPixels();
}

Mechanics::Sensor::~Sensor()
{
  for (unsigned int x = 0; x < m_numX; x++)
    delete[] m_noisyPixels[x];
  delete[] m_noisyPixels;
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
  XYZPoint uvw(col * m_pitchX - halfU, row * m_pitchY - halfV, 0);
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
  col = (uvw.x() + halfU) / m_pitchX;
  row = (uvw.y() + halfV) / m_pitchY;
}

//=========================================================
//
// noisy-pixels functions
//
//=========================================================

void Mechanics::Sensor::clearNoisyPixels()
{
  m_numNoisyPixels = 0;
  for (unsigned int x = 0; x < m_numX; x++)
    for (unsigned int y = 0; y < m_numY; y++)
      m_noisyPixels[x][y] = false;
}

void Mechanics::Sensor::setNoisyPixels(const NoiseMask& masks,
                                       unsigned int sensor_id)
{
  clearNoisyPixels();
  for (const auto& index : masks.getMaskedPixels(sensor_id)) {
    auto col = std::get<0>(index);
    auto row = std::get<1>(index);

    if (m_numX <= col)
      throw std::runtime_error("column index is out of range");
    if (m_numY <= row)
      throw std::runtime_error("row index is out of range");

    m_noisyPixels[col][row] = true;
    m_numNoisyPixels += 1;
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
       << "  Pitch-x: " << m_pitchX << "\n"
       << "  Pitch-y: " << m_pitchY << "\n"
       << "  Depth: " << m_depth << "\n"
       << "  X / X0: " << m_xox0 << "\n"
       << "  Sensitive X: " << getSensitiveX() << "\n"
       << "  Sensitive Y: " << getSensitiveY() << "\n"
       << "  PosNumX: " << getPosNumX() << "\n"
       << "  PosNumY: " << getPosNumY() << "\n"
       << "  Alignable: " << m_alignable << endl;

  cout << "  Noisy pixels (" << m_numNoisyPixels << ")" << endl;
  if (m_numNoisyPixels < 20) {
    for (unsigned int nx = 0; nx < getNumX(); nx++)
      for (unsigned int ny = 0; ny < getNumY(); ny++)
        if (m_noisyPixels[nx][ny])
          cout << "    " << nx << " : " << ny << endl;
  }
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

bool** Mechanics::Sensor::getNoiseMask() const { return m_noisyPixels; }
bool Mechanics::Sensor::getDigital() const { return m_digi; }
bool Mechanics::Sensor::getAlignable() const { return m_alignable; }
unsigned int Mechanics::Sensor::getNumX() const { return m_numX; }
unsigned int Mechanics::Sensor::getNumY() const { return m_numY; }
unsigned int Mechanics::Sensor::getNumPixels() const { return m_numX * m_numY; }
unsigned int Mechanics::Sensor::getNumNoisyPixels() const
{
  return m_numNoisyPixels;
}
double Mechanics::Sensor::getPitchX() const { return m_pitchX; }
double Mechanics::Sensor::getPitchY() const { return m_pitchY; }
double Mechanics::Sensor::getDepth() const { return m_depth; }
double Mechanics::Sensor::getXox0() const { return m_xox0; }
double Mechanics::Sensor::getSensitiveX() const { return m_sensitiveX; }
double Mechanics::Sensor::getSensitiveY() const { return m_sensitiveY; }
const Mechanics::Device* Mechanics::Sensor::getDevice() const
{
  return m_device;
}
const char* Mechanics::Sensor::getName() const { return m_name.c_str(); }
