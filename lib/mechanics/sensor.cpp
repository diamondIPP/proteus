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
                          int timestampMin,
                          int timestampMax,
                          int valueMax,
                          Scalar pitchCol,
                          Scalar pitchRow,
                          Scalar pitchTimestamp,
                          Scalar xX0)
    : m_id(id)
    , m_name(name)
    , m_numCols(numCols)
    , m_numRows(numRows)
    , m_timestampRange(timestampMin, timestampMax)
    , m_valueRange(0, valueMax)
    , m_pitchCol(pitchCol)
    , m_pitchRow(pitchRow)
    , m_pitchTimestamp(pitchTimestamp)
    , m_xX0(xX0)
    // this is geometry dependent
    , m_theta0(0)
    , m_measurement(measurement)
    // reasonable defaults for geometry-dependent properties. to be updated.
    , m_beamSlope(Vector2::Zero())
    , m_beamSlopeCov(SymMatrix2::Zero())
    , m_projPitch(Vector4::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_projBoundingBox(Volume::Empty())
{
}

void Mechanics::Sensor::addRegion(
    const std::string& name, int col_min, int col_max, int row_min, int row_max)
{
  Region region;
  region.name = name;
  // ensure that the region is bounded by the sensor size
  region.colRow = DigitalArea(DigitalRange(col_min, col_max),
                              DigitalRange(row_min, row_max));
  region.colRow = intersection(this->colRowArea(), region.colRow);
  // ensure that all regions are uniquely named and areas are exclusive
  for (const auto& other : m_regions) {
    if (other.name == region.name) {
      FAIL("region '", other.name,
           "' already exists and can not be defined again");
    }
    if (!(intersection(other.colRow, region.colRow).isEmpty())) {
      FAIL("region '", other.name, "' intersects with region '", region.name,
           "'");
    }
  }
  // region is well-defined and can be added
  m_regions.push_back(std::move(region));
}

// position of the sensor center in pixel coordinates
Vector4 Mechanics::Sensor::pixelCenter() const
{
  Vector4 c;
  c[kU] = std::round(m_numCols / Scalar(2)) - Scalar(0.5);
  c[kV] = std::round(m_numRows / Scalar(2)) - Scalar(0.5);
  c[kW] = 0;
  c[kS] = 0;
  return c;
}

Vector4 Mechanics::Sensor::pitch() const
{
  Vector4 p;
  p[kU] = m_pitchCol;
  p[kV] = m_pitchRow;
  p[kW] = 0; // TODO use thickness here to avoid singularity?
  p[kS] = m_pitchTimestamp;
  return p;
}

Vector4 Mechanics::Sensor::transformPixelToLocal(Scalar col,
                                                 Scalar row,
                                                 Scalar timestamp) const
{
  Vector4 q;
  q[kU] = col;
  q[kV] = row;
  q[kW] = 0;
  q[kS] = timestamp;
  return pitch().cwiseProduct(q - pixelCenter());
}

Vector4 Mechanics::Sensor::transformLocalToPixel(const Vector4& local) const
{
  return pixelCenter() + local.cwiseQuotient(pitch());
}

Mechanics::Sensor::Volume Mechanics::Sensor::sensitiveVolume() const
{
  // this code assumes local/global coordinates have the same ordering. this is
  // a canary to alert you that somebody is bold/stupid enough to change it.
  static_assert(kU == 0, "Congratulations, you broke the code!");
  static_assert(kV == 1, "Congratulations, you broke the code!");
  static_assert(kW == 2, "Congratulations, you broke the code!");
  static_assert(kS == 3, "Congratulations, you broke the code!");

  // digital address/timestamp is bin center, upper edge is exclusive
  auto col = colRange();
  auto row = rowRange();
  auto ts = timestampRange();
  auto lowerLeft =
      transformPixelToLocal(col.min() - 0.5, row.min() - 0.5, ts.min() - 0.5);
  auto upperRight =
      transformPixelToLocal(col.max() - 0.5, row.max() - 0.5, ts.max() - 0.5);

  Volume::AxisInterval ivU(lowerLeft[kU], upperRight[kU]);
  Volume::AxisInterval ivV(lowerLeft[kV], upperRight[kV]);
  Volume::AxisInterval ivW(lowerLeft[kW], upperRight[kW]); // TODO or thickness?
  Volume::AxisInterval ivS(lowerLeft[kS], upperRight[kS]);
  return Volume(ivU, ivV, ivW, ivS);
}

// Compute scattering angle standard deviation using the updated PDG formula
//
// Assumes that the momentum is given in GeV and |charge| = 1e.
static Scalar scatteringStdev(Scalar t, Scalar momentum, Scalar mass)
{
  // return zero scattering for invalid inputs as sensible fallback
  if (not((0 < t) and (0 < momentum))) {
    return 0;
  }
  //    beta      = pc / E
  // -> 1 / beta² = E² / (pc)²
  //              = ((pc)² + m²) / (pc)²
  //              = 1² + (m/pc)²
  auto betaInv = std::hypot(1, mass / momentum);
  // square root of the reduced material thickness, i.e. sqrt(x / (beta² X0))
  auto sqrtD = std::sqrt(t) * betaInv;
  // uses log(x²) = 2 log(x)
  auto fromThickness = sqrtD * (1 + Scalar(0.038) * 2 * std::log(sqrtD));
  // assumes momentum in GeV;
  auto fromMomentum = (Scalar(0.0136) / momentum);
  return fromMomentum * fromThickness;
}

// update projections of local properties into the global system and vice versa
void Mechanics::Sensor::updateGeometry(const Geometry& geometry)
{
  // this code assumes local/global coordinates have the same ordering. this is
  // a canary to alert you that somebody is bold/stupid enough to change it.
  static_assert(kX == kU, "Come on, are you serious?");
  static_assert(kY == kV, "Come on, are you serious?");
  static_assert(kZ == kW, "Come on, are you serious?");
  static_assert(kT == kS, "Come on, are you serious?");

  const auto& plane = geometry.getPlane(m_id);
  m_beamSlope = geometry.getBeamSlope(m_id);
  m_beamSlopeCov = geometry.getBeamSlopeCovariance(m_id);

  // update expected scattering angle
  // scaling due to non-zero incidence
  auto incidence = std::sqrt(1 + m_beamSlope.squaredNorm());
  // TODO store beam momentum + mass not energy
  // For now this assumes massless beam particles
  m_theta0 = scatteringStdev(m_xX0 * incidence, geometry.beamEnergy(), 0);

  // brute-force bounding box projection of the sensor in global coordinates by
  // transforming each corner into the global system
  auto volume = sensitiveVolume();
  Matrix<double, 4, 16> corners;
  // clang-format off
  corners <<
      Vector4(volume.min(0), volume.min(1), volume.min(2), volume.min(3)),
      Vector4(volume.max(0), volume.min(1), volume.min(2), volume.min(3)),
      Vector4(volume.min(0), volume.max(1), volume.min(2), volume.min(3)),
      Vector4(volume.max(0), volume.max(1), volume.min(2), volume.min(3)),
      Vector4(volume.min(0), volume.min(1), volume.max(2), volume.min(3)),
      Vector4(volume.max(0), volume.min(1), volume.max(2), volume.min(3)),
      Vector4(volume.min(0), volume.max(1), volume.max(2), volume.min(3)),
      Vector4(volume.max(0), volume.max(1), volume.max(2), volume.min(3)),
      Vector4(volume.min(0), volume.min(1), volume.min(2), volume.max(3)),
      Vector4(volume.max(0), volume.min(1), volume.min(2), volume.max(3)),
      Vector4(volume.min(0), volume.max(1), volume.min(2), volume.max(3)),
      Vector4(volume.max(0), volume.max(1), volume.min(2), volume.max(3)),
      Vector4(volume.min(0), volume.min(1), volume.max(2), volume.max(3)),
      Vector4(volume.max(0), volume.min(1), volume.max(2), volume.max(3)),
      Vector4(volume.min(0), volume.max(1), volume.max(2), volume.max(3)),
      Vector4(volume.max(0), volume.max(1), volume.max(2), volume.max(3));
  // clang-format on
  // convert local corners to global corners
  for (int i = 0; i < corners.cols(); ++i) {
    corners.col(i) = plane.toGlobal(corners.col(i));
  }
  // determine bounding box of the rotated volume
  m_projBoundingBox = Volume(Volume::AxisInterval(corners.row(0).minCoeff(),
                                                  corners.row(0).maxCoeff()),
                             Volume::AxisInterval(corners.row(1).minCoeff(),
                                                  corners.row(1).maxCoeff()),
                             Volume::AxisInterval(corners.row(2).minCoeff(),
                                                  corners.row(2).maxCoeff()),
                             Volume::AxisInterval(corners.row(3).minCoeff(),
                                                  corners.row(3).maxCoeff()));
  // only absolute pitch is relevant for the projection
  m_projPitch = (plane.linearToGlobal() * pitch()).cwiseAbs();
}

SymMatrix2 Mechanics::Sensor::scatteringSlopeCovariance() const
{
  SymMatrix2 cov;
  // projection from comoving frame to local frame
  cov(0, 0) = 1 + m_beamSlope[0] * m_beamSlope[0];
  cov(1, 1) = 1 + m_beamSlope[1] * m_beamSlope[1];
  cov(0, 1) = cov(1, 0) = m_beamSlope[0] * m_beamSlope[1];
  // overall scaling
  cov *= m_theta0 * m_theta0 * (1 + m_beamSlope.squaredNorm());
  return cov;
}

SymMatrix2 Mechanics::Sensor::scatteringSlopePrecision() const
{
  SymMatrix2 prec;
  // projection from comoving frame to local frame
  prec(0, 0) = 1 + m_beamSlope[1] * m_beamSlope[1];
  prec(1, 1) = 1 + m_beamSlope[0] * m_beamSlope[0];
  prec(0, 1) = prec(1, 0) = -m_beamSlope[0] * m_beamSlope[1];
  // overall scaling
  auto scale = 1 / (m_theta0 * (1 + m_beamSlope.squaredNorm()));
  prec *= scale * scale;
  return prec;
}

void Mechanics::Sensor::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "measurement: " << measurementName(m_measurement) << '\n';
  os << prefix << "col: " << colRange() << '\n';
  os << prefix << "row: " << rowRange() << '\n';
  os << prefix << "timestamp: " << timestampRange() << '\n';
  os << prefix << "value: " << valueRange() << '\n';
  os << prefix << "pitch_col: " << m_pitchCol << '\n';
  os << prefix << "pitch_row: " << m_pitchRow << '\n';
  os << prefix << "pitch_timestamp: " << m_pitchTimestamp << '\n';
  if (!m_regions.empty()) {
    os << prefix << "regions:\n";
    for (size_t iregion = 0; iregion < m_regions.size(); ++iregion) {
      const auto& region = m_regions[iregion];
      os << prefix << "  region " << iregion << ":\n";
      os << prefix << "    name: " << region.name << '\n';
      os << prefix << "    col: " << region.colRow.interval(0) << '\n';
      os << prefix << "    row: " << region.colRow.interval(1) << '\n';
    }
  }
  os << prefix << "x/X0: " << m_xX0 << '\n';
  os << prefix << "theta0: " << m_theta0 * 1000 << " mrad\n";
  os.flush();
}
