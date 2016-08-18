#ifndef SENSOR_H
#define SENSOR_H

#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Mechanics {

class Device;
class NoiseMask;

class Sensor {
public:
  Sensor(const std::string& name,
         Index numCols,
         Index numRows,
         double pitchCol,
         double pitchRow,
         double depth = 0,
         double xX0 = 0);
  Sensor(unsigned int numX,
         unsigned int numY,
         double pitchX,
         double pitchY,
         double depth,
         Device* device,
         std::string name,
         bool digi,
         bool alignable = true,
         double xox0 = 0,
         double offX = 0,
         double offY = 0,
         double offZ = 0,
         double rotX = 0,
         double rotY = 0,
         double rotZ = 0);
  ~Sensor();

  //
  // geometry related
  //
  const Transform3D& localToGlobal() const { return m_l2g; }
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
  // Geometry functions
  //
  void
  pixelToSpace(double pixX, double pixY, double& x, double& y, double& z) const;
  void
  spaceToPixel(double x, double y, double z, double& pixX, double& pixY) const;

  //
  // noise-pixels functions
  //
  void setNoisyPixels(const NoiseMask& masks, unsigned int sensor_id);
  inline bool isPixelNoisy(unsigned int x, unsigned int y) const
  {
    return m_noisyPixels[x][y];
  }

  //
  // Misc functions
  //
  void print();
  static bool sort(const Sensor* s1, const Sensor* s2);

  //
  // Get functions
  //
  bool** getNoiseMask() const;
  bool getDigital() const;
  bool getAlignable() const;
  unsigned int getNumX() const;
  unsigned int getNumY() const;
  unsigned int getNumPixels() const;
  unsigned int getPosNumX() const;
  unsigned int getPosNumY() const;
  unsigned int getNumNoisyPixels() const;
  double getPitchX() const;
  double getPitchY() const;
  double getPosPitchX() const;
  double getPosPitchY() const;
  double getDepth() const;
  double getXox0() const;
  double getSensitiveX() const;
  double getSensitiveY() const;
  double getPosSensitiveX() const;
  double getPosSensitiveY() const;
  const Device* getDevice() const;
  const char* getName() const;

private:
  void rotateToGlobal(double& x, double& y, double& z) const;
  void rotateToSensor(double& x, double& y, double& z) const;
  void clearNoisyPixels();

private:
  Transform3D m_l2g;
  const Index m_numCols, m_numRows; // number of columns and rows
  const double m_pitchCol, m_pitchRow; // pitch along column and row direction
  const double m_depth; // sensor thickness
  const double m_xox0; // X/X0 (thickness in radiation lengths)
  const std::string m_name;
  const Device* m_device;
  bool m_digi;
  bool m_alignable;    // if sensor is to be aligned
  unsigned int m_numNoisyPixels; // total number of noisy pixels
  bool** m_noisyPixels;          // 2D array of noisy-pixels
};

} // namespace Mechanics

#endif // SENSOR_H
