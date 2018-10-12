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

/** Pixel sensor with digital and local geometry information.
 *
 * To define the sensor and its orientation in space, three different
 * coordinate systems are used:
 *
 * 1.  The pixel coordinates are defined along the column and row
 *     axis of the pixel matrix. Coordinates are measured in numbers of digital
 *     pixels. Pixel boundaries are located on integer numbers, i.e. the
 *     (0,0) pixel has column and row position in the [0,1) range and unit
 *     area. Please note that for pixel address (0,0) the pixel center is
 *     located at (0.5,0.5). This shift must be considered when converting
 *     addresses to pixel positions. The total sensitive sensor area is
 *     [0,numberCols)x[0,numberRows).
 * 2.  The local metric coordinates are also defined along the column and row
 *     axis of the pixel matrix but scaled with the pixel pitch to have the
 *     same units as the global coordinates. Local coordinates (u,v,w)
 *     correspond to the column, row, and normal axis, where the normal axis
 *     is defined such that the coordinate system is right-handed. The origin
 *     is located at the center of the sensitive area but always on a pixel
 *     boundary.
 * 3.  The global coordinate system has the same units as the local coordinate
 *     system with coordinates (x,y,z);
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

  /** One-dimensional range of possible values, e.g. for time and value. */
  using IntRange = Utils::Interval<int>;
  /** Two-dimensional area type, e.g. for size and region-of-interest. */
  using Area = Utils::Box<2, double>;
  /** Three-dimensional bounding box type for projected volume. */
  using Volume = Utils::Box<3, double>;
  /** A named region on the sensor. */
  struct Region {
    std::string name;
    Area areaPixel = Area::Empty();
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
         int timeMin,
         int timeMax,
         int valueMax,
         double pitchCol,
         double pitchRow,
         double thickness,
         double xX0);

  // identification
  Index id() const { return m_id; }
  const std::string& name() const { return m_name; }
  Measurement measurement() const { return m_measurement; }

  // local digital properties
  Index numCols() const { return m_numCols; }
  Index numRows() const { return m_numRows; }
  const IntRange& timeRange() const { return m_timeRange; }
  const IntRange& valueRange() const { return m_valueRange; }
  // local physical properties
  double pitchCol() const { return m_pitchCol; }
  double pitchRow() const { return m_pitchRow; }
  double thickness() const { return m_thickness; }
  double xX0() const { return m_xX0; }

  bool hasRegions() const { return !m_regions.empty(); }
  const std::vector<Region>& regions() const { return m_regions; }

  void setMaskedPixels(const std::set<ColumnRow>& pixels);
  const Utils::DenseMask& pixelMask() const { return m_pixelMask; }

  /** Transform pixel matrix position to local coordinates. */
  Vector2 transformPixelToLocal(const Vector2& cr) const;
  /** Transform local coordinates to pixel matrix position. */
  Vector2 transformLocalToPixel(const Vector2& uv) const;

  /** Sensitive area in pixel coordinates. */
  Area sensitiveAreaPixel() const;
  /** Sensitive area in local coordinates. */
  Area sensitiveAreaLocal() const;
  /** Sensitive volume in local coordinates. */
  Volume sensitiveVolumeLocal() const;

  /** Beam slope in the local coordinate system. */
  auto beamSlope() const { return m_beamSlope; }
  /** Beam slope covariance in the local coordinate system. */
  auto beamCovariance() const { return m_beamCov; }
  /** Beam slope divergence in the local coordinate system. */
  auto beamDivergence() const { return extractStdev(m_beamCov); }

  /** Bounding box volume of the detector in the global system. */
  auto projectedEnvelope() const { return m_projEnvelope; }
  /** Projected pitch in the global system. */
  auto projectedPitch() const { return m_projPitch; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  void addRegion(const std::string& name,
                 int col_min,
                 int col_max,
                 int row_min,
                 int row_max);

  // local information
  Index m_id;
  std::string m_name;
  Index m_numCols, m_numRows; // number of columns and rows
  IntRange m_timeRange;
  IntRange m_valueRange;
  double m_pitchCol, m_pitchRow; // pitch along column and row direction
  double m_thickness;            // sensor thickness
  double m_xX0;                  // X/X0 (thickness in radiation lengths)
  Measurement m_measurement;
  std::vector<Region> m_regions;
  Utils::DenseMask m_pixelMask;

  // geometry-dependent information
  Vector2 m_beamSlope;  // global beam slope in the local system
  SymMatrix2 m_beamCov; // global beam slope covariance in the local system
  Vector3 m_projPitch;
  Volume m_projEnvelope;

  friend class Device;
};

} // namespace Mechanics

#endif // PT_SENSOR_H
