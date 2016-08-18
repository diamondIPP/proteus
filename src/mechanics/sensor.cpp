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
    , m_offX(offX)
    , m_offY(offY)
    , m_offZ(offZ)
    , m_rotX(rotX)
    , m_rotY(rotY)
    , m_rotZ(rotZ)
    , m_alignable(alignable)
    , m_sensitiveX(pitchX * numX)
    , m_sensitiveY(pitchY * numY)
    , m_numNoisyPixels(0)
{
  assert(device && "Sensor: need to link the sensor back to a device.");

  calculateRotation();

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

//=========================================================
//
// Set functions
//
//=========================================================
void Mechanics::Sensor::setOffX(double offset) { m_offX = offset; }
void Mechanics::Sensor::setOffY(double offset) { m_offY = offset; }
void Mechanics::Sensor::setOffZ(double offset) { m_offZ = offset; }
void Mechanics::Sensor::setRotX(double rotation)
{
  m_rotX = rotation;
  calculateRotation();
}
void Mechanics::Sensor::setRotY(double rotation)
{
  m_rotY = rotation;
  calculateRotation();
}
void Mechanics::Sensor::setRotZ(double rotation)
{
  m_rotZ = rotation;
  calculateRotation();
}

//=========================================================
//
// geometry functions
//
//=========================================================
void Mechanics::Sensor::applyRotation(double& x,
                                      double& y,
                                      double& z,
                                      bool invert) const
{
  const double point[3] = {x, y, z};
  double* rotated[3] = {&x, &y, &z};

  // Use the sensor rotation matrix to transform
  for (int i = 0; i < 3; i++) {
    *(rotated[i]) = 0;
    for (int j = 0; j < 3; j++) {
      if (!invert)
        *(rotated[i]) += m_rotation[i][j] * point[j];
      else
        *(rotated[i]) += m_unRotate[i][j] * point[j];
    }
  }

  // std::cout << *(rotated[0]) << std::endl;
  // if( m_rotation[0][1]==1){
  // if(*(rotated[0]) == 0){
  // std::cout << "Coordinates were (" << x <<";"<< y <<";"<< z << ")"<<
  // std::endl;
  // std::cout << "They are: (" << *(rotated[0]) <<";"<< *(rotated[1]) <<";"<<
  // *(rotated[2]) <<")" << std::endl;
  //    std::cout << "" << std::endl;
  //}
  //}
}

void Mechanics::Sensor::calculateRotation()
{
  const double rx = m_rotX;
  const double ry = m_rotY;
  const double rz = m_rotZ;
  m_rotation[0][0] = cos(ry) * cos(rz);
  m_rotation[0][1] = -cos(rx) * sin(rz) + sin(rx) * sin(ry) * cos(rz);
  m_rotation[0][2] = sin(rx) * sin(rz) + cos(rx) * sin(ry) * cos(rz);
  m_rotation[1][0] = cos(ry) * sin(rz);
  m_rotation[1][1] = cos(rx) * cos(rz) + sin(rx) * sin(ry) * sin(rz);
  m_rotation[1][2] = -sin(rx) * cos(rz) + cos(rx) * sin(ry) * sin(rz);
  m_rotation[2][0] = -sin(ry);
  m_rotation[2][1] = sin(rx) * cos(ry);
  m_rotation[2][2] = cos(rx) * cos(ry);

  // Transpose of a rotation matrix is its inverse
  for (unsigned int i = 0; i < 3; i++)
    for (unsigned int j = 0; j < 3; j++)
      m_unRotate[i][j] = m_rotation[j][i];

  m_normalX = 0, m_normalY = 0, m_normalZ = 1;
  applyRotation(m_normalX, m_normalY, m_normalZ);
}

void Mechanics::Sensor::rotateToGlobal(double& x, double& y, double& z) const
{
  applyRotation(x, y, z);
}

void Mechanics::Sensor::rotateToSensor(double& x, double& y, double& z) const
{
  applyRotation(x, y, z, true);
}

void Mechanics::Sensor::pixelToSpace(
    double pixX, double pixY, double& x, double& y, double& z) const
{
  if (pixX >= m_numX && pixY >= m_numY)
    throw "Sensor: requested pixel out of range";

  x = 0, y = 0, z = 0;

  const double halfX = getSensitiveX() / 2.0;
  const double halfY = getSensitiveY() / 2.0;

  x = pixX * m_pitchX; // To the middle of the pixel
  x -= halfX;         // Origin on center of sensor

  y = pixY * m_pitchY;
  y -= halfY;
  rotateToGlobal(x, y, z);

  x += getOffX();
  y += getOffY();
  z += getOffZ();
}

void Mechanics::Sensor::spaceToPixel(
    double x, double y, double z, double& pixX, double& pixY) const
{
  const double halfX = getSensitiveX() / 2.0;
  const double halfY = getSensitiveY() / 2.0;

  // Subtract sensor offsets
  x -= getOffX();
  y -= getOffY();
  z -= getOffZ();

  rotateToSensor(x, y, z);

  x += halfX;
  y += halfY;

  double mapX = x / m_pitchX;
  double mapY = y / m_pitchY;

  pixX = mapX;
  pixY = mapY;
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
       << "  Pos: " << m_offX << " , " << m_offY << " , " << m_offZ << "\n"
       << "  Rot: " << m_rotX << " , " << m_rotY << " , " << m_rotZ << "\n"
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
void Mechanics::Sensor::getGlobalOrigin(double& x, double& y, double& z) const
{
  x = getOffX();
  y = getOffY();
  z = getOffZ();
}

void Mechanics::Sensor::getNormalVector(double& x, double& y, double& z) const
{
  x = m_normalX;
  y = m_normalY;
  z = m_normalZ;
}

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
double Mechanics::Sensor::getOffX() const { return m_offX; }
double Mechanics::Sensor::getOffY() const { return m_offY; }
double Mechanics::Sensor::getOffZ() const { return m_offZ; }
double Mechanics::Sensor::getRotX() const { return m_rotX; }
double Mechanics::Sensor::getRotY() const { return m_rotY; }
double Mechanics::Sensor::getRotZ() const { return m_rotZ; }
double Mechanics::Sensor::getSensitiveX() const { return m_sensitiveX; }
double Mechanics::Sensor::getSensitiveY() const { return m_sensitiveY; }
const Mechanics::Device* Mechanics::Sensor::getDevice() const
{
  return m_device;
}
const char* Mechanics::Sensor::getName() const { return m_name.c_str(); }
