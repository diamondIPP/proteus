#ifndef PT_SENSOR_H
#define PT_SENSOR_H

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "utils/definitions.h"
#include "utils/densemask.h"
#include "utils/interval.h"

namespace Mechanics {

class Geometry;

/** Pixel sensor with digital pixel and local geometry information.
 *
 * To define the sensor and its orientation in space, three different
 * coordinate systems are used:
 *
 * *   The pixel coordinates are defined along the column, row, and timestamp
 *     axis of the pixel matrix. Coordinates are given as digital values.
 *     The pixel centers correspond to integer numbers, i.e. the (0,0) pixel
 *     covers an area from [-0.5,0.5) along each coordinate and the total
 *     sensitive sensor area covers [-0.5,numberCols-0.5)x[0.5,numberRows-0.5).
 * *   The local metric coordinates are also defined along the column, row,
 *     and timestamp axis of the pixel matrix but scaled with the pitch to
 *     the same units as the global coordinates. Local coordinates (u,v,w,s)
 *     correspond to the column, row, normal, and timestamp axis, where the
 *     normal axis is defined such that the coordinate system is right-handed.
 *     The origin is located at the lower-left edge of the central pixel.
 * *   The global coordinate system has the same units as the local coordinate
 *     system with coordinates (x,y,z,t).
 *
 * This class handles only local information, i.e. the pixel and local
 * coordinate system. The placement of sensors in the global coordinate
 * system and the corresponding transformations are handled in the geometry
 * module.
 */
class Sensor {
public:
  /** Measurement type of the sensor. */
  enum class Measurement {
    PixelBinary, // generic pixel detector with binary measurement
    PixelTot,    // generic pixel detector with time-over-treshold measurement
    Ccpdv4Binary // HVCMOS ccpd version 4 with address mapping and binary pixel
  };
  static Measurement measurementFromName(const std::string& name);
  static std::string measurementName(Measurement measurement);

  /** One-dimensional range of digital values, e.g. for time and value. */
  using DigitalRange = Utils::Interval<int>;
  /** Two-dimensional area of digital matrix positions, i.e. column and row. */
  using DigitalArea = Utils::Box<2, int>;
  /** Four-dimensional bounding box type for projected volume. */
  using Volume = Utils::Box<4, double>;
  /** A named region on the sensor. */
  struct Region {
    std::string name;
    DigitalArea colRow = DigitalArea::Empty();
  };

  /** Construct with an empty transformation (local = global) and empty mask.
   *
   * This is the minimal configuration required to have a usable Sensor.
   */
  Sensor(Index id,
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
         Scalar thickness,
         Scalar xX0);

  // identification
  Index id() const { return m_id; }
  const std::string& name() const { return m_name; }
  Measurement measurement() const { return m_measurement; }

  // local digital properties
  DigitalRange colRange() const { return {0, static_cast<int>(m_numCols)}; }
  DigitalRange rowRange() const { return {0, static_cast<int>(m_numRows)}; }
  DigitalArea colRowArea() const { return {colRange(), rowRange()}; }
  DigitalRange timestampRange() const { return m_timestampRange; }
  DigitalRange valueRange() const { return m_valueRange; }
  bool hasRegions() const { return !m_regions.empty(); }
  const std::vector<Region>& regions() const { return m_regions; }
  const Utils::DenseMask& pixelMask() const { return m_pixelMask; }

  // local physical properties
  Scalar pitchCol() const { return m_pitchCol; }
  Scalar pitchRow() const { return m_pitchRow; }
  Scalar pitchTimestamp() const { return m_pitchTimestamp; }
  Scalar thickness() const { return m_thickness; }
  Scalar xX0() const { return m_xX0; }

  /** Pitch in local coordinates. */
  Vector4 pitch() const;
  /** Transform pixel matrix position to local coordinates. */
  Vector4 transformPixelToLocal(Scalar col, Scalar row, Scalar timestamp) const;
  /** Transform local coordinates to pixel matrix position. */
  Vector4 transformLocalToPixel(const Vector4& local) const;

  /** Sensitive volume in local coordinates. */
  Volume sensitiveVolume() const;
  /** Beam slope in the local coordinate system. */
  const Vector2& beamSlope() const { return m_beamSlope; }
  /** Beam slope covariance in the local coordinate system. */
  const SymMatrix2& beamSlopeCovariance() const { return m_beamSlopeCov; }

  /** Projected pitch in the global system. */
  const Vector4& projectedPitch() const { return m_projPitch; }
  /** Bounding box of the detector in the global system. */
  const Volume& projectedBoundingBox() const { return m_projBoundingBox; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  void addRegion(const std::string& name,
                 int col_min,
                 int col_max,
                 int row_min,
                 int row_max);
  void updateGeometry(const Geometry& geometry);
  Vector4 pixelCenter() const;

  // local information
  Index m_id;
  std::string m_name;
  Index m_numCols, m_numRows; // number of columns and rows
  DigitalRange m_timestampRange;
  DigitalRange m_valueRange;
  Scalar m_pitchCol, m_pitchRow; // digital col/row address to metric location
  Scalar m_pitchTimestamp;       // digital timestamp to metric time
  Scalar m_thickness;            // sensor thickness
  Scalar m_xX0;                  // X/X0 (thickness in radiation lengths)
  Measurement m_measurement;
  std::vector<Region> m_regions;
  Utils::DenseMask m_pixelMask;
  // geometry-dependent information
  Vector2 m_beamSlope;       // beam slope in the local system
  SymMatrix2 m_beamSlopeCov; // beam slope covariance in the local system
  Vector4 m_projPitch;
  Volume m_projBoundingBox;

  friend class Device;
};

} // namespace Mechanics

#endif // PT_SENSOR_H
