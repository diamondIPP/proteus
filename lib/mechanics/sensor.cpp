#include "sensor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>

#include "mechanics/geometry.h"
#include "tracking/propagation.h"
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
    : m_id(id)
    , m_name(name)
    , m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_thickness(thickness)
    , m_xX0(xX0)
    // reasonable defaults for global properties that require the full geometry.
    // this should be updated later on by the device.
    , m_beamSlope(0.0, 0.0)
    , m_beamDivergence(0.00125, 0.00125)
    , m_projPitch(pitchCol, pitchRow, 0)
    , m_projEnvelope(sensitiveVolumeLocal())
    , m_measurement(measurement)
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

XYPoint Mechanics::Sensor::transformPixelToLocal(const XYPoint& cr) const
{
  // place sensor center on a pixel edge in the middle of the sensitive area
  return XYPoint(m_pitchCol * (cr.x() - std::round(m_numCols / 2.0)),
                 m_pitchRow * (cr.y() - std::round(m_numRows / 2.0)));
}

XYPoint Mechanics::Sensor::transformLocalToPixel(const XYPoint& uv) const
{
  // place sensor center on a pixel edge in the middle of the sensitive area
  return XYPoint((uv.x() / m_pitchCol) + std::round(m_numCols / 2.0),
                 (uv.y() / m_pitchRow) + std::round(m_numRows / 2.0));
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaPixel() const
{
  return Area(Area::AxisInterval(0, static_cast<double>(m_numCols)),
              Area::AxisInterval(0, static_cast<double>(m_numRows)));
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaLocal() const
{
  auto pix = sensitiveAreaPixel();
  // calculate local sensitive area
  XYPoint lowerLeft = transformPixelToLocal({pix.min(0), pix.min(1)});
  XYPoint upperRight = transformPixelToLocal({pix.max(0), pix.max(1)});
  return Area(Area::AxisInterval(lowerLeft.x(), upperRight.x()),
              Area::AxisInterval(lowerLeft.y(), upperRight.y()));
}

Mechanics::Sensor::Volume Mechanics::Sensor::sensitiveVolumeLocal() const
{
  auto area = sensitiveAreaLocal();
  // TODO 2017-10 msmk: which way does the thickess grow, w or -w
  return Volume(area.interval(0), area.interval(1),
                Volume::AxisInterval(0, m_thickness));
}

void Mechanics::Sensor::setMaskedPixels(const std::set<ColumnRow>& pixels)
{
  m_pixelMask = Utils::DenseMask(pixels);
}

void Mechanics::Sensor::updateBeam(const Plane& plane,
                                   const Vector3& directionXYZ,
                                   const Vector2& stdevXY)
{
  // project global beam parameters into the local plane
  Vector3 dir = Transpose(plane.rotation) * directionXYZ;
  Matrix2 jac =
      Tracking::jacobianSlopeSlope(directionXYZ, Transpose(plane.rotation));
  SymMatrix2 covGlobal;
  covGlobal(0, 0) = stdevXY[0] * stdevXY[0];
  covGlobal(1, 1) = stdevXY[1] * stdevXY[1];
  covGlobal(0, 1) = covGlobal(1, 0) = 0;
  SymMatrix2 covLocal = Similarity(jac, covGlobal);
  m_beamSlope = Vector2(dir[0] / dir[2], dir[1] / dir[2]);
  m_beamDivergence = sqrt(covLocal.Diagonal());
}

void Mechanics::Sensor::updateProjections(const Plane& plane)
{
  // project pitch vector into the global system
  m_projPitch = plane.rotation * Vector3(m_pitchCol, m_pitchRow, 0);
  // only interested in absolute values not in direction
  m_projPitch[0] = std::abs(m_projPitch[0]);
  m_projPitch[1] = std::abs(m_projPitch[1]);
  m_projPitch[2] = std::abs(m_projPitch[2]);

  // project sensor envelope into the global xyz system
  Area pix = sensitiveAreaLocal();
  // TODO 2016-08 msmk: find a smarter way to this, but its Friday
  // TODO 2017-10 msmk: which way does the thickess grow, z or -z
  // transform each corner of the sensitive box,
  Vector3 iii = plane.toGlobal(Vector3(pix.min(0), pix.min(1), 0));
  Vector3 iia = plane.toGlobal(Vector3(pix.min(0), pix.min(1), m_thickness));
  Vector3 iai = plane.toGlobal(Vector3(pix.min(0), pix.max(1), 0));
  Vector3 iaa = plane.toGlobal(Vector3(pix.min(0), pix.max(1), m_thickness));
  Vector3 aii = plane.toGlobal(Vector3(pix.max(0), pix.min(1), 0));
  Vector3 aia = plane.toGlobal(Vector3(pix.max(0), pix.min(1), m_thickness));
  Vector3 aai = plane.toGlobal(Vector3(pix.max(0), pix.max(1), 0));
  Vector3 aaa = plane.toGlobal(Vector3(pix.max(0), pix.max(1), m_thickness));
  double xmin = std::min(
      {iii[0], iia[0], iai[0], iaa[0], aii[0], aia[0], aai[0], aaa[0]});
  double xmax = std::max(
      {iii[0], iia[0], iai[0], iaa[0], aii[0], aia[0], aai[0], aaa[0]});
  double ymin = std::min(
      {iii[1], iia[1], iai[1], iaa[1], aii[1], aia[1], aai[1], aaa[1]});
  double ymax = std::max(
      {iii[1], iia[1], iai[1], iaa[1], aii[1], aia[1], aai[1], aaa[1]});
  double zmin = std::min(
      {iii[2], iia[2], iai[2], iaa[2], aii[2], aia[2], aai[2], aaa[2]});
  double zmax = std::max(
      {iii[2], iia[2], iai[2], iaa[2], aii[2], aia[2], aai[2], aaa[2]});
  m_projEnvelope =
      Volume(Volume::AxisInterval(xmin, xmax), Volume::AxisInterval(ymin, ymax),
             Volume::AxisInterval(zmin, zmax));
}

void Mechanics::Sensor::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "measurement: " << measurementName(m_measurement) << '\n';
  os << prefix << "cols: " << m_numCols << '\n';
  os << prefix << "rows: " << m_numRows << '\n';
  os << prefix << "pitch_col: " << m_pitchCol << '\n';
  os << prefix << "pitch_row: " << m_pitchRow << '\n';
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
  os << prefix << "thickness: " << m_thickness << '\n';
  os << prefix << "x/X0: " << m_xX0 << '\n';
  os.flush();
}
