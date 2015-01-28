#ifndef SENSOR_H
#define SENSOR_H

#include <vector>
#include <string>

namespace Mechanics {
  
  class Device;
  
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
	   bool alignable=true,	   
	   double xox0=0,
	   double offX=0,
	   double offY=0,
	   double offZ=0,
	   double rotX=0,
	   double rotY=0,
	   double rotZ=0);

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
    void pixelToSpace(double pixX, double pixY,
		      double& x, double& y, double& z) const;
    void spaceToPixel(double x, double y, double z,
		      double& pixX, double& pixY) const;
    
    //
    // noise-pixels functions
    //
    void addNoisyPixel(unsigned int x, unsigned int y);
    void clearNoisyPixels();    

    inline bool isPixelNoisy(unsigned int x, unsigned int y) const {
      return _noisyPixels[x][y];
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
    void applyRotation(double& x, double& y, double& z, bool invert=false) const;
    void calculateRotation(); // Calculates the rotation matricies and normal vector
    
  private:
    const unsigned int _numX; // number of columns (local x-direction)
    const unsigned int _numY; // number of rows (local y-direction)
    const double _pitchX;     // pitch along x (col) (250 um for FEI4)
    const double _pitchY;     // pitch along y (row) ( 50 um for FEI4)
    const double _depth; // sensor thickness
    const Device* _device;
    std::string _name;
    bool _digi;
    const double _xox0; // X/X0
    double _offX; // translation in X
    double _offY; // translation in Y 
    double _offZ; // translation in Z
    double _rotX; // rotation angle (rad) around Global X-axis
    double _rotY; // rotation angle (rad) around Global Y-axis
    double _rotZ; // rotation angle (rad) around Global Z-axis
    bool _alignable; // if sensor is to be aligned
    const double _sensitiveX;
    const double _sensitiveY;
    unsigned int _numNoisyPixels; // total number of noisy pixels
    bool** _noisyPixels;          // 2D array of noisy-pixels
    
    double _rotation[3][3]; // The rotation matrix for the plane
    double _unRotate[3][3]; // Invert the rotation
    double _normalX;
    double _normalY;
    double _normalZ;   
    
  }; // end of class
  
} // end of namespace

#endif // SENSOR_H
