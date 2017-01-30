#include "sensor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <string>

// mapping between measurement enum and names
struct MeasurementName {
  Mechanics::Sensor::Measurement measurement;
  const char* name;
};
static const MeasurementName MEAS_NAMES[] = {
    {Mechanics::Sensor::Measurement::PixelBinary, "pixel_binary"},
    {Mechanics::Sensor::Measurement::PixelTot, "pixel_tot"},
    {Mechanics::Sensor::Measurement::Ccpdv4Binary, "ccpdv4_binary"}};

Mechanics::Sensor::Measurement
Mechanics::Sensor::measurementFromName(const std::string& name)
{
  for (auto mn = std::begin(MEAS_NAMES); mn != std::end(MEAS_NAMES); ++mn) {
    if (name.compare(mn->name) == 0) {
      return mn->measurement;
    }
  }
  throw std::invalid_argument("invalid sensor measurement name '" + name + "'");
}

std::string Mechanics::Sensor::measurementName(Measurement measurement)
{
  for (auto mn = std::begin(MEAS_NAMES); mn != std::end(MEAS_NAMES); ++mn) {
    if (mn->measurement == measurement) {
      return mn->name;
    }
  }
  // This fall-back should never happen
  throw std::runtime_error("Sensor::Measurement: invalid measurement");
  return "invalid_measurement";
}

Mechanics::Sensor::Sensor(Index id,
                          const std::string& name,
                          Measurement measurement,
                          Index numCols,
                          Index numRows,
                          double pitchCol,
                          double pitchRow,
                          double thickness,
                          double xX0)
    : m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_thickness(thickness)
    , m_xX0(xX0)
    , m_measurement(measurement)
    , m_id(id)
    , m_name(name)
    , m_pixelMask(numCols * numRows, false)
{
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaPixel() const
{
  return Area(Area::AxisInterval(0, static_cast<double>(m_numCols)),
              Area::AxisInterval(0, static_cast<double>(m_numRows)));
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaLocal() const
{
  XYPoint lowerLeft = transformPixelToLocal(XYPoint(0, 0));
  XYPoint upperRight = transformPixelToLocal(XYPoint(m_numCols, m_numRows));
  return Area(Area::AxisInterval(lowerLeft.x(), upperRight.x()),
              Area::AxisInterval(lowerLeft.y(), upperRight.y()));
}

Mechanics::Sensor::Area Mechanics::Sensor::projectedEnvelopeXY() const
{
  // TODO 2016-08 msmk: find a smarter way to this, but its Friday
  // transform each corner of the sensitive rectangle
  XYZPoint minMin = transformPixelToGlobal(XYPoint(0, 0));
  XYZPoint minMax = transformPixelToGlobal(XYPoint(0, m_numRows));
  XYZPoint maxMin = transformPixelToGlobal(XYPoint(m_numCols, 0));
  XYZPoint maxMax = transformPixelToGlobal(XYPoint(m_numCols, m_numRows));

  std::array<double, 4> xs = {{minMin.x(), minMax.x(), maxMin.x(), maxMax.x()}};
  std::array<double, 4> ys = {{minMin.y(), minMax.y(), maxMin.y(), maxMax.y()}};

  return Area(Area::AxisInterval(*std::min_element(xs.begin(), xs.end()),
                                 *std::max_element(xs.begin(), xs.end())),
              Area::AxisInterval(*std::min_element(ys.begin(), ys.end()),
                                 *std::max_element(ys.begin(), ys.end())));
}

Vector2 Mechanics::Sensor::projectedPitchXY() const
{
  XYZVector pitchXYZ = m_l2g * XYZVector(m_pitchCol, m_pitchRow, 0);
  return Vector2(std::abs(pitchXYZ.x()), std::abs(pitchXYZ.y()));
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

// from 2d plane point to 3d space point
static inline XYZPoint planeToSpace(const Transform3D& transform,
                                    const XYPoint& uv)
{
  XYZPoint tmp(uv.x(), uv.y(), 0);
  tmp = transform * tmp;
  return tmp;
}

// from 3d space point to 2d plane point. 3d point must be on the plane.
static inline XYPoint spaceToPlane(const Transform3D& transform,
                                   const XYZPoint& xyz)
{
  XYZPoint tmp(transform * xyz);
  return XYPoint(tmp.x(), tmp.y());
}

XYPoint Mechanics::Sensor::transformPixelToLocal(const XYPoint& cr) const
{
  return XYPoint(m_pitchCol * (cr.x() - std::round(m_numCols / 2)),
                 m_pitchRow * (cr.y() - std::round(m_numRows / 2)));
}

XYPoint Mechanics::Sensor::transformLocalToPixel(const XYPoint& uv) const
{
  return XYPoint((uv.x() / m_pitchCol) + std::round(m_numCols / 2),
                 (uv.y() / m_pitchRow) + std::round(m_numRows / 2));
}

XYZPoint Mechanics::Sensor::transformLocalToGlobal(const XYPoint& uv) const
{
  return planeToSpace(m_l2g, uv);
}

XYPoint Mechanics::Sensor::transformGlobalToLocal(const XYZPoint& xyz) const
{
  return spaceToPlane(m_g2l, xyz);
}

XYZPoint Mechanics::Sensor::transformPixelToGlobal(const XYPoint& cr) const
{
  return planeToSpace(m_l2g, transformPixelToLocal(cr));
}

XYPoint Mechanics::Sensor::transformGlobalToPixel(const XYZPoint& xyz) const
{
  return transformLocalToPixel(spaceToPlane(m_g2l, xyz));
}

void Mechanics::Sensor::pixelToSpace(
    double c, double r, double& x, double& y, double& z) const
{
  transformPixelToGlobal(XYPoint(c, r)).GetCoordinates(x, y, z);
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

void Mechanics::Sensor::setMaskedPixels(const std::set<ColumnRow>& pixels)
{
  // reset possible existing mask
  std::fill(m_pixelMask.begin(), m_pixelMask.end(), false);

  for (auto it = pixels.begin(); it != pixels.end(); ++it) {
    auto col = std::get<0>(*it);
    auto row = std::get<1>(*it);

    if (m_numCols <= col)
      throw std::runtime_error("column index is out of range");
    if (m_numRows <= row)
      throw std::runtime_error("row index is out of range");

    m_pixelMask[linearPixelIndex(col, row)] = true;
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
  os << prefix << "id: " << m_id << '\n';
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "measurement: " << measurementName(m_measurement) << '\n';
  os << prefix << "columns: " << m_numCols << '\n';
  os << prefix << "rows: " << m_numRows << '\n';
  os << prefix << "pitch column: " << m_pitchCol << '\n';
  os << prefix << "pitch row: " << m_pitchRow << '\n';
  os << prefix << "thickness: " << m_thickness << '\n';
  os << prefix << "x/X0: " << m_xX0 << '\n';
  os.flush();
}
