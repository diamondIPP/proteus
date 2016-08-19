#include "sensor.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using std::cout;
using std::endl;

Mechanics::Sensor::Sensor(const std::string& name,
                          Index numCols,
                          Index numRows,
                          double pitchCol,
                          double pitchRow,
                          bool isDigital,
                          double depth,
                          double xX0)
    : m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_depth(depth)
    , m_xX0(xX0)
    , m_name(name)
    , m_noiseMask(numCols * numRows, false)
    , m_isDigital(isDigital)
{
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

XYPoint Mechanics::Sensor::transformLocalToPixel(const XYZPoint& uvw) const
{
  return {(uvw.x() / m_pitchCol) + (m_numCols / 2),
          (uvw.y() / m_pitchRow) + (m_numRows / 2)};
}

XYPoint Mechanics::Sensor::transformGlobalToPixel(const XYZPoint& xyz) const
{
  return transformLocalToPixel(globalToLocal() * xyz);
}

XYZPoint Mechanics::Sensor::transformPixelToLocal(double col, double row) const
{
  return {m_pitchCol * (col - (m_numCols / 2)),
          m_pitchRow * (row - (m_numRows / 2)),
          0};
}

XYZPoint Mechanics::Sensor::transformGlobalToLocal(const XYZPoint& xyz) const
{
  return globalToLocal() * xyz;
}

XYZPoint Mechanics::Sensor::transformPixelToGlobal(double col, double row) const
{
  return localToGlobal() * transformPixelToLocal(col, row);
}

XYZPoint Mechanics::Sensor::transformLocalToGlobal(const XYZPoint& uvw) const
{
  return localToGlobal() * uvw;
}

void Mechanics::Sensor::pixelToSpace(
    double col, double row, double& x, double& y, double& z) const
{
  transformPixelToGlobal(col, row).GetCoordinates(x, y, z);
}

void Mechanics::Sensor::spaceToPixel(
    double x, double y, double z, double& col, double& row) const
{
  transformGlobalToPixel({x, y, z}).GetCoordinates(col, row);
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
void Mechanics::Sensor::print() const
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
       << "  X / X0: " << m_xX0 << "\n"
       << "  Sensitive X: " << getSensitiveX() << "\n"
       << "  Sensitive Y: " << getSensitiveY() << "\n"
       << "  PosNumX: " << getPosNumX() << "\n"
       << "  PosNumY: " << getPosNumY() << "\n";

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
  XYZVector pitch = localToGlobal() * XYZVector(m_pitchCol, m_pitchRow, 0);
  return std::abs(pitch.x());
}

double Mechanics::Sensor::getPosPitchY() const
{
  XYZVector pitch = localToGlobal() * XYZVector(m_pitchCol, m_pitchRow, 0);
  return std::abs(pitch.y());
}

double Mechanics::Sensor::getPosSensitiveX() const
{
  XYZVector size =
      localToGlobal() * XYZVector(getSensitiveX(), getSensitiveY(), 0);
  return std::abs(size.x());
}

double Mechanics::Sensor::getPosSensitiveY() const
{
  XYZVector size =
      localToGlobal() * XYZVector(getSensitiveX(), getSensitiveY(), 0);
  return std::abs(size.y());
}
