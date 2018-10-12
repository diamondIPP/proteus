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
                          int timeMin,
                          int timeMax,
                          int valueMax,
                          double pitchCol,
                          double pitchRow,
                          double thickness,
                          double xX0)
    : m_id(id)
    , m_name(name)
    , m_numCols(numCols)
    , m_numRows(numRows)
    , m_timeRange(timeMin, timeMax)
    , m_valueRange(0, valueMax)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_thickness(thickness)
    , m_xX0(xX0)
    , m_measurement(measurement)
    // reasonable defaults for geometry-dependent properties. to be updated.
    , m_beamSlope(Vector2::Zero())
    , m_beamCov(SymMatrix2::Zero())
    , m_projPitch(pitchCol, pitchRow, 0)
    , m_projEnvelope(sensitiveVolumeLocal())
{
}

void Mechanics::Sensor::addRegion(
    const std::string& name, int col_min, int col_max, int row_min, int row_max)
{
  Region region;
  region.name = name;
  // ensure that the region is bounded by the sensor size
  region.areaPixel = Area(Area::AxisInterval(col_min, col_max),
                          Area::AxisInterval(row_min, row_max));
  region.areaPixel = intersection(this->sensitiveAreaPixel(), region.areaPixel);
  // ensure that all regions are uniquely named and areas are exclusive
  for (const auto& other : m_regions) {
    if (other.name == region.name) {
      FAIL("region '", other.name,
           "' already exists and can not be defined again");
    }
    if (!(intersection(other.areaPixel, region.areaPixel).isEmpty())) {
      FAIL("region '", other.name, "' intersects with region '", region.name,
           "'");
    }
  }
  // region is well-defined and can be added
  m_regions.push_back(std::move(region));
}

Vector2 Mechanics::Sensor::transformPixelToLocal(const Vector2& cr) const
{
  // place sensor center on a pixel edge in the middle of the sensitive area
  return {m_pitchCol * (cr[0] - std::round(m_numCols / 2.0f)),
          m_pitchRow * (cr[1] - std::round(m_numRows / 2.0f))};
}

Vector2 Mechanics::Sensor::transformLocalToPixel(const Vector2& uv) const
{
  // place sensor center on a pixel edge in the middle of the sensitive area
  return {(uv[0] / m_pitchCol) + std::round(m_numCols / 2.0f),
          (uv[1] / m_pitchRow) + std::round(m_numRows / 2.0f)};
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaPixel() const
{
  return Area(Area::AxisInterval(0.0, static_cast<double>(m_numCols)),
              Area::AxisInterval(0.0, static_cast<double>(m_numRows)));
}

Mechanics::Sensor::Area Mechanics::Sensor::sensitiveAreaLocal() const
{
  auto pix = sensitiveAreaPixel();
  // calculate local sensitive area
  Vector2 lowerLeft = transformPixelToLocal({pix.min(0), pix.min(1)});
  Vector2 upperRight = transformPixelToLocal({pix.max(0), pix.max(1)});
  return Area(Area::AxisInterval(lowerLeft[0], upperRight[0]),
              Area::AxisInterval(lowerLeft[1], upperRight[1]));
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

void Mechanics::Sensor::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "measurement: " << measurementName(m_measurement) << '\n';
  os << prefix << "cols: " << m_numCols << '\n';
  os << prefix << "rows: " << m_numRows << '\n';
  os << prefix << "time: " << m_timeRange << '\n';
  os << prefix << "value: " << m_valueRange << '\n';
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
