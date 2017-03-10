#include "sensor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Sensor)

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
                          double xX0,
                          const std::vector<Region>& regions)
    : m_sensitiveAreaPixel(
          Area(Area::AxisInterval(0, static_cast<double>(numCols)),
               Area::AxisInterval(0, static_cast<double>(numRows))))
    // place sensor center on a pixel edge in the middle of the sensitive area
    , m_sensitiveCenterPixel(std::round(numCols / 2), std::round(numRows / 2))
    , m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_thickness(thickness)
    , m_xX0(xX0)
    , m_measurement(measurement)
    , m_id(id)
    , m_name(name)
    , m_regions(regions)
{
  // ensure that all regions are bounded by the sensor size
  for (auto& region : m_regions) {
    region.areaPixel =
        intersection(region.areaPixel, Area(Area::AxisInterval(0, numCols),
                                            Area::AxisInterval(0, numRows)));
  }
  // ensure that all regions are uniquely named and have exclusive areas
  for (size_t i = 0; i < m_regions.size(); ++i) {
    for (size_t j = 0; j < m_regions.size(); ++j) {
      if (i == j)
        continue;
      const auto& r0 = m_regions[i];
      const auto& r1 = m_regions[j];
      if (r0.name == r1.name) {
        FAIL("region ", i, " and region ", j, " share the same name");
      }
      if (!intersection(r0.areaPixel, r1.areaPixel).isEmpty()) {
        FAIL("region '", r0.name, "' intersects with region '", r1.name, "'");
      }
    }
  }
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaLocal() const
{
  Area pix = sensitiveAreaPixel();
  XYPoint lowerLeft = transformPixelToLocal(XYPoint(pix.min(0), pix.min(1)));
  XYPoint upperRight = transformPixelToLocal(XYPoint(pix.max(0), pix.max(1)));
  return Area(Area::AxisInterval(lowerLeft.x(), upperRight.x()),
              Area::AxisInterval(lowerLeft.y(), upperRight.y()));
}

Mechanics::Sensor::Area Mechanics::Sensor::projectedEnvelopeXY() const
{
  Area pix = sensitiveAreaPixel();
  // TODO 2016-08 msmk: find a smarter way to this, but its Friday
  // transform each corner of the sensitive rectangle
  XYZPoint minMin = transformPixelToGlobal(XYPoint(pix.min(0), pix.min(1)));
  XYZPoint minMax = transformPixelToGlobal(XYPoint(pix.min(0), pix.max(1)));
  XYZPoint maxMin = transformPixelToGlobal(XYPoint(pix.max(0), pix.min(1)));
  XYZPoint maxMax = transformPixelToGlobal(XYPoint(pix.max(0), pix.max(1)));

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
  Translation3D shiftToCenter(-m_sensitiveCenterPixel.x(),
                              -m_sensitiveCenterPixel.y(),
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
  return XYPoint(m_pitchCol * (cr.x() - m_sensitiveCenterPixel.x()),
                 m_pitchRow * (cr.y() - m_sensitiveCenterPixel.y()));
}

XYPoint Mechanics::Sensor::transformLocalToPixel(const XYPoint& uv) const
{
  return XYPoint((uv.x() / m_pitchCol), (uv.y() / m_pitchRow)) +
         m_sensitiveCenterPixel;
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
  m_pixelMask = Utils::DenseMask(pixels);
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
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "measurement: " << measurementName(m_measurement) << '\n';
  os << prefix << "cols: " << m_numCols << '\n';
  os << prefix << "rows: " << m_numRows << '\n';
  os << prefix << "pitch_col: " << m_pitchCol << '\n';
  os << prefix << "pitch_row: " << m_pitchRow << '\n';
  os << prefix << "thickness: " << m_thickness << '\n';
  os << prefix << "x/X0: " << m_xX0 << '\n';
  if (!m_regions.empty()) {
    os << prefix << "regions:\n";
    for (size_t iregion = 0; iregion < m_regions.size(); ++iregion) {
      const auto& region = m_regions[iregion];
      os << prefix << "  region " << iregion << ":\n";
      os << prefix << "    name: " << region.name << '\n';
      os << prefix << "    col: " << region.areaPixel.interval(0) << '\n';
      os << prefix << "    row: " << region.areaPixel.interval(1) << '\n';
    }
  }
  os << prefix << "sensitive area:\n";
  os << prefix << "  col: " << sensitiveAreaPixel().interval(0) << '\n';
  os << prefix << "  row: " << sensitiveAreaPixel().interval(1) << '\n';
  os << prefix << "  u: " << sensitiveAreaLocal().interval(0) << '\n';
  os << prefix << "  b: " << sensitiveAreaLocal().interval(1) << '\n';
  os.flush();
}
