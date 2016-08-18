#ifndef SENSOR_H
#define SENSOR_H

#include <string>
#include <vector>

namespace Mechanics {

class Device;
class NoiseMask;

class Sensor {
public:
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
  // Set functions
  //
  void setOffX(double offset);
  void setOffY(double offset);
  void setOffZ(double offset);
  void setRotX(double rotation);
  void setRotY(double rotation);
  void setRotZ(double rotation);

  //
  // Geometry functions
  //
  void rotateToGlobal(double& x, double& y, double& z) const;
  void rotateToSensor(double& x, double& y, double& z) const;
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
  double getOffX() const;
  double getOffY() const;
  double getOffZ() const;
  double getRotX() const;
  double getRotY() const;
  double getRotZ() const;
  double getSensitiveX() const;
  double getSensitiveY() const;
  double getPosSensitiveX() const;
  double getPosSensitiveY() const;
  const Device* getDevice() const;
  const char* getName() const;
  void getGlobalOrigin(double& x, double& y, double& z) const;
  void getNormalVector(double& x, double& y, double& z) const;

private:
  void clearNoisyPixels();
  void
  applyRotation(double& x, double& y, double& z, bool invert = false) const;
  void
  calculateRotation(); // Calculates the rotation matricies and normal vector

private:
  const unsigned int m_numX; // number of columns (local x-direction)
  const unsigned int m_numY; // number of rows (local y-direction)
  const double m_pitchX;     // pitch along x (col) (250 um for FEI4)
  const double m_pitchY;     // pitch along y (row) ( 50 um for FEI4)
  const double m_depth;      // sensor thickness
  const Device* m_device;
  std::string m_name;
  bool m_digi;
  const double m_xox0; // X/X0
  double m_offX;       // translation in X
  double m_offY;       // translation in Y
  double m_offZ;       // translation in Z
  double m_rotX;       // rotation angle (rad) around Global X-axis
  double m_rotY;       // rotation angle (rad) around Global Y-axis
  double m_rotZ;       // rotation angle (rad) around Global Z-axis
  bool m_alignable;    // if sensor is to be aligned
  const double m_sensitiveX;
  const double m_sensitiveY;
  unsigned int m_numNoisyPixels; // total number of noisy pixels
  bool** m_noisyPixels;          // 2D array of noisy-pixels

  double m_rotation[3][3]; // The rotation matrix for the plane
  double m_unRotate[3][3]; // Invert the rotation
  double m_normalX;
  double m_normalY;
  double m_normalZ;
};

} // namespace Mechanics

#endif // SENSOR_H
