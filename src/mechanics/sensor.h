#ifndef SENSOR_H
#define SENSOR_H

#include <array>
#include <cstdint>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "utils/definitions.h"

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
  enum Measurement {
    PIXEL_BINARY, // generic pixel detector with binary measurement
    PIXEL_TOT,    // generic pixel detector with time-over-treshold measurement
    CCPDV4_BINARY // HVCMOS ccpd version 4 with address mapping and binary pixel
  };
  static Measurement measurementFromName(const std::string& name);
  static std::string measurementName(Measurement measurement);

  /** Construct with an empty transformation (local = global) and no noise.
   *
   * This is the minimal configuration required to have a usable Sensor.
   */
  Sensor(const std::string& name,
         Measurement measurement,
         Index numCols,
         Index numRows,
         double pitchCol,
         double pitchRow,
         double thickness,
         double xX0);

  //
  // properties
  //
  const std::string& name() const { return m_name; }
  Measurement measurement() const { return m_measurement; }
  Index numCols() const { return m_numCols; }
  Index numRows() const { return m_numRows; }
  Index numPixels() const { return m_numCols * m_numRows; }
  double pitchCol() const { return m_pitchCol; }
  double pitchRow() const { return m_pitchRow; }
  double thickness() const { return m_thickness; }
  double xX0() const { return m_xX0; }
  /** Sensitive area in pixel coordinates [col_min,col_max,row_min,row_max]. */
  std::array<double, 4> sensitiveAreaPixel() const;
  /** Sensitive area in local coordinates [u_min,u_max,v_min,v_max]. */
  std::array<double, 4> sensitiveAreaLocal() const;
  /** Sensitive envelope in the global xy-plane [x_min,x_max,y_min,y_max]. */
  std::array<double, 4> sensitiveEnvelopeGlobal() const;

  //
  // geometry related
  //
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
  XYPoint transformLocalToPixel(const XYZPoint& uvw) const;
  XYPoint transformGlobalToPixel(const XYZPoint& xyz) const;
  XYZPoint transformPixelToLocal(double c, double r) const;
  XYZPoint transformPixelToLocal(const XYPoint& cr) const;
  XYZPoint transformGlobalToLocal(const XYZPoint& xyz) const;
  XYZPoint transformPixelToGlobal(double c, double r) const;
  XYZPoint transformPixelToGlobal(const XYPoint& cr) const;
  XYZPoint transformLocalToGlobal(const XYZPoint& uvw) const;
  // \deprecated Use transformations above
  void
  pixelToSpace(double pixX, double pixY, double& x, double& y, double& z) const;
  void
  spaceToPixel(double x, double y, double z, double& pixX, double& pixY) const;

  //
  // noise-pixels functions
  //
  bool isPixelNoisy(Index col, Index row) const
  {
    return m_noiseMask[linearPixelIndex(col, row)];
  }
  void setNoisyPixels(const std::set<ColumnRow>& pixels);

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
  // row major indices for the pixel masks
  size_t linearPixelIndex(Index col, Index row) const
  {
    return m_numCols * col + row;
  }

private:
  Transform3D m_l2g, m_g2l;
  Index m_numCols, m_numRows;    // number of columns and rows
  double m_pitchCol, m_pitchRow; // pitch along column and row direction
  double m_thickness;            // sensor thickness
  double m_xX0;                  // X/X0 (thickness in radiation lengths)
  Measurement m_measurement;
  std::string m_name;
  std::vector<bool> m_noiseMask;
};

} // namespace Mechanics

#endif // SENSOR_H
