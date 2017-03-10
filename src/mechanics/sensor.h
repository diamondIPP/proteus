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

/** Pixel sensor with digital and geometry information.
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

  /** Two-dimensional area type, e.g. for size and region-of-interest. */
  using Area = Utils::Box<2, double>;
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
         double pitchCol,
         double pitchRow,
         double thickness,
         double xX0,
         const std::vector<Region>& regions = std::vector<Region>());

  // identification
  Index id() const { return m_id; }
  const std::string& name() const { return m_name; }

  // local properties
  Measurement measurement() const { return m_measurement; }
  Index numCols() const { return m_numCols; }
  Index numRows() const { return m_numRows; }
  Index numPixels() const { return m_numCols * m_numRows; }
  double pitchCol() const { return m_pitchCol; }
  double pitchRow() const { return m_pitchRow; }
  double thickness() const { return m_thickness; }
  double xX0() const { return m_xX0; }

  bool hasRegions() const { return !m_regions.empty(); }
  const std::vector<Region>& regions() const { return m_regions; }

  /** Sensitive area in pixel coordinates. */
  const Area& sensitiveAreaPixel() const { return m_sensitiveAreaPixel; }
  /** Sensitive area in local coordinates. */
  Area sensitiveAreaLocal() const;
  /** Projected envelope in the xy-plane of the sensitive area. */
  Area projectedEnvelopeXY() const;
  /** Projected pitch along the global x and y axes. */
  Vector2 projectedPitchXY() const;

  // geometry
  XYZPoint origin() const;
  XYZVector normal() const;
  const Transform3D& localToGlobal() const { return m_l2g; }
  const Transform3D& globalToLocal() const { return m_g2l; }
  /** Construct a transformation for pixel col/row to global xyz coordinates. */
  Transform3D constructPixelToGlobal() const;
  void setLocalToGlobal(const Transform3D& l2g);

  //
  // transformations between different coordinate systems
  //
  XYPoint transformPixelToLocal(const XYPoint& cr) const;
  XYPoint transformLocalToPixel(const XYPoint& uv) const;
  XYZPoint transformLocalToGlobal(const XYPoint& uv) const;
  XYPoint transformGlobalToLocal(const XYZPoint& xyz) const;
  XYZPoint transformPixelToGlobal(const XYPoint& cr) const;
  XYPoint transformGlobalToPixel(const XYZPoint& xyz) const;
  // \deprecated Use transformations above
  void
  pixelToSpace(double pixX, double pixY, double& x, double& y, double& z) const;
  void
  spaceToPixel(double x, double y, double z, double& pixX, double& pixY) const;

  //
  // noise-pixels functions
  //
  void setMaskedPixels(const std::set<ColumnRow>& pixels);
  const Utils::DenseMask& pixelMask() const { return m_pixelMask; }

  //
  // Misc functions
  //
  void print(std::ostream& os, const std::string& prefix = std::string()) const;
  static bool sort(const Sensor* s1, const Sensor* s2);

  // \deprecated For backward compatibility.
  unsigned int getNumX() const { return m_numCols; }
  unsigned int getNumY() const { return m_numRows; }
  unsigned int getNumPixels() const { return m_numCols * m_numRows; }
  unsigned int getPosNumX() const;
  unsigned int getPosNumY() const;
  double getPitchX() const { return m_pitchCol; }
  double getPitchY() const { return m_pitchRow; }
  double getPosPitchX() const;
  double getPosPitchY() const;
  double getDepth() const { return m_thickness; }
  double getXox0() const { return m_xX0; }
  double getSensitiveX() const { return m_numCols * m_pitchCol; }
  double getSensitiveY() const { return m_numRows * m_pitchRow; }
  double getPosSensitiveX() const;
  double getPosSensitiveY() const;
  double getOffX() const;
  double getOffY() const;
  double getOffZ() const;
  void getGlobalOrigin(double& x, double& y, double& z) const;
  void getNormalVector(double& x, double& y, double& z) const;
  const std::string& getName() const { return m_name; }

private:
  Area m_sensitiveAreaPixel; // sensitive (useful) area inside the matrix
  XYVector m_sensitiveCenterPixel; // center position of the sensitive area
  Transform3D m_l2g, m_g2l;
  Index m_numCols, m_numRows;    // number of columns and rows
  double m_pitchCol, m_pitchRow; // pitch along column and row direction
  double m_thickness;            // sensor thickness
  double m_xX0;                  // X/X0 (thickness in radiation lengths)
  Measurement m_measurement;
  Index m_id;
  std::string m_name;
  std::vector<Region> m_regions;
  Utils::DenseMask m_pixelMask;
};

} // namespace Mechanics

#endif // PT_SENSOR_H
