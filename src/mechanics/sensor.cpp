#include "sensor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <ostream>
#include <string>

Mechanics::Sensor::Sensor(const std::string& name,
                          Index numCols,
                          Index numRows,
                          double pitchCol,
                          double pitchRow,
                          bool isDigital,
                          double thickness,
                          double xX0)
    : m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_thickness(thickness)
    , m_xX0(xX0)
    , m_name(name)
    , m_noiseMask(numCols * numRows, false)
    , m_isDigital(isDigital)
{
}

// geometry related methods

XYZPoint Mechanics::Sensor::origin() const
{
  return XYZPoint(m_l2g.Translation().Vect());
}

XYZVector Mechanics::Sensor::normal() const
{
  XYZVector W, U, V;
  m_l2g.Rotation().GetComponents(U, V, W);
  return W;
}

Transform3D Mechanics::Sensor::constructPixelToGlobal() const
{
  // clang-format off
  Translation3D shiftToCenter(-std::round(m_numCols / 2),
                              -std::round(m_numRows / 2),
                              0);
  Rotation3D scalePitch(m_pitchCol, 0, 0,
                        0, m_pitchRow, 0,
                        0, 0, m_thickness);
  // clang-format on
  return m_l2g * scalePitch * shiftToCenter;
}

void Mechanics::Sensor::setLocalToGlobal(const Transform3D& l2g)
{
  m_l2g = l2g;
  m_g2l = l2g.Inverse();
}

XYPoint Mechanics::Sensor::transformLocalToPixel(const XYZPoint& uvw) const
{
  return XYPoint((uvw.x() / m_pitchCol) + std::round(m_numCols / 2),
                 (uvw.y() / m_pitchRow) + std::round(m_numRows / 2));
}

XYPoint Mechanics::Sensor::transformGlobalToPixel(const XYZPoint& xyz) const
{
  return transformLocalToPixel(globalToLocal() * xyz);
}

XYZPoint Mechanics::Sensor::transformPixelToLocal(double c, double r) const
{
  return XYZPoint(m_pitchCol * (c - std::round(m_numCols / 2)),
                  m_pitchRow * (r - std::round(m_numRows / 2)),
                  0);
}

XYZPoint Mechanics::Sensor::transformPixelToLocal(const XYPoint& cr) const
{
  return transformPixelToLocal(cr.x(), cr.y());
}

XYZPoint Mechanics::Sensor::transformGlobalToLocal(const XYZPoint& xyz) const
{
  return globalToLocal() * xyz;
}

XYZPoint Mechanics::Sensor::transformPixelToGlobal(double c, double r) const
{
  return localToGlobal() * transformPixelToLocal(c, r);
}

XYZPoint Mechanics::Sensor::transformPixelToGlobal(const XYPoint& cr) const
{
  return localToGlobal() * transformPixelToLocal(cr.x(), cr.y());
}

XYZPoint Mechanics::Sensor::transformLocalToGlobal(const XYZPoint& uvw) const
{
  return localToGlobal() * uvw;
}

void Mechanics::Sensor::pixelToSpace(
    double c, double r, double& x, double& y, double& z) const
{
  transformPixelToGlobal(c, r).GetCoordinates(x, y, z);
}

void Mechanics::Sensor::spaceToPixel(
    double x, double y, double z, double& c, double& r) const
{
  transformGlobalToPixel(XYZPoint(x, y, z)).GetCoordinates(c, r);
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

bool Mechanics::Sensor::sort(const Sensor* s1, const Sensor* s2)
{
  return (s1->getOffZ() < s2->getOffZ());
}

//=========================================================
//
// get functions
//
//=========================================================

std::array<double, 4> Mechanics::Sensor::sensitiveAreaPixel() const
{
  return {0, static_cast<double>(m_numCols), 0, static_cast<double>(m_numRows)};
}

std::array<double, 4> Mechanics::Sensor::sensitiveAreaLocal() const
{
  auto lowerLeft = transformPixelToLocal(0, 0);
  auto upperRight = transformPixelToLocal(m_numCols, m_numRows);
  return {lowerLeft.x(), upperRight.x(), lowerLeft.y(), upperRight.y()};
}

std::array<double, 4> Mechanics::Sensor::sensitiveEnvelopeGlobal() const
{
  // TODO 2016-08 msmk: find a smarter way to this, but its Friday
  // transform each corner of the sensitive rectangle
  auto minMin = transformPixelToGlobal(0, 0);
  auto minMax = transformPixelToGlobal(0, m_numRows);
  auto maxMin = transformPixelToGlobal(m_numCols, 0);
  auto maxMax = transformPixelToGlobal(m_numCols, m_numRows);

  std::array<double, 4> xs = {minMin.x(), minMax.x(), maxMin.x(), maxMax.x()};
  std::array<double, 4> ys = {minMin.y(), minMax.y(), maxMin.y(), maxMax.y()};

  return {*std::min_element(xs.begin(), xs.end()),
          *std::max_element(xs.begin(), xs.end()),
          *std::min_element(ys.begin(), ys.end()),
          *std::max_element(ys.begin(), ys.end())};
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

void Mechanics::Sensor::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "columns: " << m_numCols << '\n';
  os << prefix << "rows: " << m_numRows << '\n';
  os << prefix << "pitch column: " << m_pitchCol << '\n';
  os << prefix << "pitch row: " << m_pitchRow << '\n';
  os << prefix << "thickness: " << m_thickness << '\n';
  os << prefix << "x/X0: " << m_xX0 << '\n';
  os.flush();
}
