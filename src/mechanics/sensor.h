#ifndef SENSOR_H
#define SENSOR_H

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Mechanics {

/** Pixel sensor with digital and geometry information.
 *
 * To define the sensor its local coordinate and its orientation in space
 * three different coordinate systems are defined:
 *
 * 1.  The digital pixel coordinates are defined along the column and row
 *     axis of the pixel matrix. Coordinates are measured in digital pixel
 *     addresses, i.e. columns and rows. The sensitive sensor area is located
 *     from [0,numberCols) and [0,numberRows). Each has unit area, e.g.
 *     the positions inside the pixel at (0,0) have a column and row position
 *     in the [0,1) range.
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
  /** Construct with an empty transformation (local = global) and no noise.
   *
   * This is the minimal configuration required to have a usable Sensor.
   */
  Sensor(const std::string& name,
         Index numCols,
         Index numRows,
         double pitchCol,
         double pitchRow,
         bool isDigital,
         double depth = 0,
         double xX0 = 0);

  //
  // geometry related
  //
  const Transform3D& localToGlobal() const { return m_l2g; }
  Transform3D globalToLocal() const { return m_l2g.Inverse(); }
  XYZPoint origin() const { return XYZPoint(m_l2g.Translation().Vect()); }
  XYZVector normal() const;
  void setLocalToGlobal(const Transform3D& l2g) { m_l2g = l2g; }
  // \deprecated For backward compatibility.
  double getOffX() const;
  double getOffY() const;
  double getOffZ() const;
  void getGlobalOrigin(double& x, double& y, double& z) const;
  void getNormalVector(double& x, double& y, double& z) const;

  //
  // transformations between different coordinate systems
  //
  XYPoint transformLocalToPixel(const XYZPoint& uvw) const;
  XYPoint transformGlobalToPixel(const XYZPoint& xyz) const;
  XYZPoint transformPixelToLocal(double col, double row) const;
  XYZPoint transformGlobalToLocal(const XYZPoint& xyz) const;
  XYZPoint transformPixelToGlobal(double col, double row) const;
  XYZPoint transformLocalToGlobal(const XYZPoint& uvw) const;
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
  void print() const;
  static bool sort(const Sensor* s1, const Sensor* s2);

  //
  // Get functions
  //
  bool getDigital() const { return m_isDigital; }
  bool getAlignable() const { return true; }
  unsigned int getNumX() const { return m_numCols; }
  unsigned int getNumY() const { return m_numRows; }
  unsigned int getNumPixels() const { return m_numCols * m_numRows; }
  unsigned int getPosNumX() const;
  unsigned int getPosNumY() const;
  double getPitchX() const { return m_pitchCol; }
  double getPitchY() const { return m_pitchRow; }
  double getPosPitchX() const;
  double getPosPitchY() const;
  double getDepth() const { return m_depth; }
  double getXox0() const { return m_xX0; }
  double getSensitiveX() const { return m_numCols * m_pitchCol; }
  double getSensitiveY() const { return m_numRows * m_pitchRow; }
  double getPosSensitiveX() const;
  double getPosSensitiveY() const;
  const std::string& getName() const { return m_name; }

private:
  // row major indices for the pixel masks
  size_t linearPixelIndex(Index col, Index row) const
  {
    return m_numCols * col + row;
  }

private:
  Transform3D m_l2g;
  const Index m_numCols, m_numRows;    // number of columns and rows
  const double m_pitchCol, m_pitchRow; // pitch along column and row direction
  const double m_depth;                // sensor thickness
  const double m_xX0;                 // X/X0 (thickness in radiation lengths)
  const std::string m_name;
  std::vector<bool> m_noiseMask;
  bool m_isDigital;
};

} // namespace Mechanics

#endif // SENSOR_H
