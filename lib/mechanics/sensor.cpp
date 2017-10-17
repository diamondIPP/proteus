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
    : m_numCols(numCols)
    , m_numRows(numRows)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    // place sensor center on a pixel edge in the middle of the sensitive area
    , m_sensitiveCenterPixel(std::round(numCols / 2), std::round(numRows / 2))
    , m_sensitiveAreaPixel(
          Area(Area::AxisInterval(0, static_cast<double>(numCols)),
               Area::AxisInterval(0, static_cast<double>(numRows))))
    , m_projPitchXY(pitchCol, pitchRow)
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
  // calculate local sensitive area
  XYPoint lowerLeft = transformPixelToLocal(
      XYPoint(m_sensitiveAreaPixel.min(0), m_sensitiveAreaPixel.min(1)));
  XYPoint upperRight = transformPixelToLocal(
      XYPoint(m_sensitiveAreaPixel.max(0), m_sensitiveAreaPixel.max(1)));
  m_sensitiveAreaLocal =
      Area(Area::AxisInterval(lowerLeft.x(), upperRight.x()),
           Area::AxisInterval(lowerLeft.y(), upperRight.y()));
  // global envelope requires geometry information; must be set by device
  m_projEnvelopeXY = m_sensitiveAreaLocal;
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

//=========================================================
//
// noisy-pixels functions
//
//=========================================================

void Mechanics::Sensor::setMaskedPixels(const std::set<ColumnRow>& pixels)
{
  m_pixelMask = Utils::DenseMask(pixels);
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
