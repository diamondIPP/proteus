#include "sensor.h"

#include <cassert>
#include <math.h>
#include <iostream>
#include <string>

#include "device.h"

using std::cout;
using std::endl;

Mechanics::Sensor::Sensor(unsigned int numX,
			  unsigned int numY,
			  double pitchX,
			  double pitchY,
			  double depth,
			  Device* device,
			  std::string name,
			  bool digi,
			  bool alignable,
			  double xox0,
			  double offX,
			  double offY,
			  double offZ,
			  double rotX,
			  double rotY,
			  double rotZ) :
  _numX(numX),
  _numY(numY),
  _pitchX(pitchX),
  _pitchY(pitchY),
  _depth(depth),
  _device(device),
  _name(name),
  _digi(digi),
  _xox0(xox0),
  _offX(offX),
  _offY(offY),
  _offZ(offZ),
  _rotX(rotX),
  _rotY(rotY),
  _rotZ(rotZ),
  _alignable(alignable),
  _sensitiveX(pitchX*numX),
  _sensitiveY(pitchY*numY),
  _numNoisyPixels(0)
{
  assert(device && "Sensor: need to link the sensor back to a device.");
  
  calculateRotation();
  
  _noisyPixels = new bool*[_numX];
  for(unsigned int x=0; x<_numX; x++){ 
    bool *row = new bool[_numY];
    _noisyPixels[x] = row;
  }
  clearNoisyPixels();
}

Mechanics::Sensor::~Sensor(){
  for(unsigned int x=0; x<_numX; x++)
    delete[] _noisyPixels[x];
  delete[] _noisyPixels;
}

//=========================================================
//
// Set functions
//
//=========================================================
void Mechanics::Sensor::setOffX(double offset) { _offX = offset; }
void Mechanics::Sensor::setOffY(double offset) { _offY = offset; }
void Mechanics::Sensor::setOffZ(double offset) { _offZ = offset; }
void Mechanics::Sensor::setRotX(double rotation) { _rotX = rotation; calculateRotation(); }
void Mechanics::Sensor::setRotY(double rotation) { _rotY = rotation; calculateRotation(); }
void Mechanics::Sensor::setRotZ(double rotation) { _rotZ = rotation; calculateRotation(); }

//=========================================================
//
// geometry functions
//
//=========================================================
void Mechanics::Sensor::applyRotation(double& x,
				      double& y,
				      double& z,
				      bool invert) const {
  const double point[3] = {x, y, z};
  double* rotated[3] = {&x, &y, &z};
  
  // Use the sensor rotation matrix to transform
  for (int i=0; i<3; i++) {
    *(rotated[i]) = 0;
    for (int j=0; j<3; j++){
      if (!invert) *(rotated[i]) += _rotation[i][j] * point[j];
      else         *(rotated[i]) += _unRotate[i][j] * point[j];      
    }
  }

  //std::cout << *(rotated[0]) << std::endl;
  //if( _rotation[0][1]==1){ 
  //if(*(rotated[0]) == 0){
  //std::cout << "Coordinates were (" << x <<";"<< y <<";"<< z << ")"<< std::endl; 
  //std::cout << "They are: (" << *(rotated[0]) <<";"<< *(rotated[1]) <<";"<< *(rotated[2]) <<")" << std::endl;
  //    std::cout << "" << std::endl;
  //}
  //}
}

void Mechanics::Sensor::calculateRotation() {
  const double rx = _rotX;
  const double ry = _rotY;
  const double rz = _rotZ;
  _rotation[0][0] = cos(ry) * cos(rz);
  _rotation[0][1] = -cos(rx) * sin(rz) + sin(rx) * sin(ry) * cos(rz);
  _rotation[0][2] = sin(rx) * sin(rz) + cos(rx) * sin(ry) * cos(rz);
  _rotation[1][0] = cos(ry) * sin(rz);
  _rotation[1][1] = cos(rx) * cos(rz) + sin(rx) * sin(ry) * sin(rz);
  _rotation[1][2] = -sin(rx) * cos(rz) + cos(rx) * sin(ry) * sin(rz);
  _rotation[2][0] = -sin(ry);
  _rotation[2][1] = sin(rx) * cos(ry);
  _rotation[2][2] = cos(rx) * cos(ry);

  // Transpose of a rotation matrix is its inverse
  for (unsigned int i = 0; i < 3; i++)
    for (unsigned int j = 0; j < 3; j++)
      _unRotate[i][j] = _rotation[j][i];

  _normalX = 0, _normalY = 0, _normalZ = 1;
  applyRotation(_normalX, _normalY, _normalZ);
}


void Mechanics::Sensor::rotateToGlobal(double& x, double& y, double& z) const{
  applyRotation(x, y, z);
}

void Mechanics::Sensor::rotateToSensor(double& x, double& y, double& z) const {
  applyRotation(x, y, z, true);
}

void Mechanics::Sensor::pixelToSpace(double pixX, double pixY,
				     double& x, double& y, double& z) const
{
  if (pixX >= _numX && pixY >= _numY)
    throw "Sensor: requested pixel out of range";

  x = 0, y = 0, z = 0;

  const double halfX = getSensitiveX() / 2.0;
  const double halfY = getSensitiveY() / 2.0;

  x = pixX * _pitchX; // To the middle of the pixel
  x -= halfX; // Origin on center of sensor

  y = pixY * _pitchY;
  y -= halfY;
  rotateToGlobal(x, y, z);

  x += getOffX();
  y += getOffY();
  z += getOffZ();
}

void Mechanics::Sensor::spaceToPixel(double x, double y, double z,
				     double& pixX, double& pixY) const
{
  const double halfX = getSensitiveX() / 2.0;
  const double halfY = getSensitiveY() / 2.0;

  // Subtract sensor offsets
  x -= getOffX();
  y -= getOffY();
  z -= getOffZ();

  rotateToSensor(x, y, z);

  x += halfX;
  y += halfY;

  double mapX = x / _pitchX;
  double mapY = y / _pitchY;

  pixX = mapX;
  pixY = mapY;
}

//=========================================================
//
// noisy-pixels functions
//
//=========================================================
void Mechanics::Sensor::addNoisyPixel(unsigned int x, unsigned int y){
  assert(x < _numX && y < _numY && "Storage: tried to add noisy pixel outside sensor");
  _noisyPixels[x][y] = true;
  _numNoisyPixels++;
}

void Mechanics::Sensor::clearNoisyPixels(){
  _numNoisyPixels=0;
  for(unsigned int x=0; x<_numX; x++)
    for(unsigned int y=0; y<_numY; y++)
      _noisyPixels[x][y] = false;
}

//=========================================================
//
// Misc functions
//
//=========================================================
void Mechanics::Sensor::print(){
  cout << "\nSENSOR:\n"
       << "  Name: '" << _name << "'\n"
       << "  Pos: " << _offX << " , " << _offY << " , " << _offZ << "\n"
       << "  Rot: " << _rotX << " , " << _rotY << " , " << _rotZ << "\n"
       << "  Cols: " << getNumX() << "\n"
       << "  Rows: " << getNumY() << "\n"
       << "  Pitch-x: " << _pitchX << "\n"
       << "  Pitch-y: " << _pitchY << "\n"
       << "  Depth: " << _depth << "\n"
       << "  X / X0: " << _xox0 << "\n"
       << "  Sensitive X: " << getSensitiveX() << "\n"
       << "  Sensitive Y: " << getSensitiveY() << "\n"
       << "  PosNumX: " << getPosNumX() << "\n"
       << "  PosNumY: " << getPosNumY() << "\n"
       << "  Alignable: " << _alignable << endl;
  
  cout << "  Noisy pixels (" << _numNoisyPixels << ")" << endl;
  if( _numNoisyPixels < 20 ){
    for (unsigned int nx=0; nx<getNumX(); nx++)
      for (unsigned int ny=0; ny<getNumY(); ny++)
        if (_noisyPixels[nx][ny]) cout << "    " << nx << " : " << ny << endl;
  }
}

bool Mechanics::Sensor::sort(const Sensor* s1, const Sensor* s2){
  return (s1->getOffZ() < s2->getOffZ());
}

//=========================================================
//
// get functions
//
//=========================================================
void Mechanics::Sensor::getGlobalOrigin(double& x, double& y, double& z) const {
  x = getOffX();
  y = getOffY();
  z = getOffZ();
}

void Mechanics::Sensor::getNormalVector(double& x, double& y, double& z) const {
  x = _normalX;
  y = _normalY;
  z = _normalZ;
}

unsigned int Mechanics::Sensor::getPosNumX() const {
  return (unsigned int)((getPosSensitiveX()/getPosPitchX())+0.5);
}

unsigned int Mechanics::Sensor::getPosNumY() const {
  return (unsigned int)((getPosSensitiveY()/getPosPitchY())+0.5);
}

double Mechanics::Sensor::getPosPitchX() const { 
  double pitch = 0;
  double x, y, z;
  // Make a vector pointing along the pixel's x-axis
  x = getPitchX();
  y = 0;
  z = 0;
  // Get it's x-projection
  rotateToGlobal(x, y, z);
  pitch += fabs(x);
  // Again with the pixel's y-axis
  x = 0;
  y = getPitchY();
  z = 0;
  rotateToGlobal(x, y, z);
  pitch += fabs(x); 
  // Return the sum of the x-projection
  return pitch;
}

double Mechanics::Sensor::getPosPitchY() const { 
  double pitch = 0;
  double x, y, z;
  x = getPitchX();
  y = 0;
  z = 0;
  rotateToGlobal(x, y, z);
  pitch += fabs(y);
  x = 0;
  y = getPitchY();
  z = 0;
  rotateToGlobal(x, y, z);
  pitch += fabs(y);
  return pitch;
}

double Mechanics::Sensor::getPosSensitiveX() const {
  double size = 0;
  double x, y, z;
  x = getSensitiveX();
  y = 0;
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(x);

  x = 0;
  y = getSensitiveY();
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(x);

  return size;
}

double Mechanics::Sensor::getPosSensitiveY() const {
  double size = 0;
  double x, y, z;
  x = getSensitiveX();
  y = 0;
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(y);
  x = 0;
  y = getSensitiveY();
  z = 0;
  rotateToGlobal(x, y, z);
  size += fabs(y);
  return size;
}

bool** Mechanics::Sensor::getNoiseMask() const { return _noisyPixels; }
bool Mechanics::Sensor::getDigital() const { return _digi; }
bool Mechanics::Sensor::getAlignable() const { return _alignable; }
unsigned int Mechanics::Sensor::getNumX() const { return _numX; }
unsigned int Mechanics::Sensor::getNumY() const { return _numY; }
unsigned int Mechanics::Sensor::getNumNoisyPixels() const { return _numNoisyPixels; }
double Mechanics::Sensor::getPitchX() const { return _pitchX; }
double Mechanics::Sensor::getPitchY() const { return _pitchY; }
double Mechanics::Sensor::getDepth() const { return _depth; }
double Mechanics::Sensor::getXox0() const { return _xox0; }
double Mechanics::Sensor::getOffX() const { return _offX; }
double Mechanics::Sensor::getOffY() const { return _offY; }
double Mechanics::Sensor::getOffZ() const { return _offZ; }
double Mechanics::Sensor::getRotX() const { return _rotX; }
double Mechanics::Sensor::getRotY() const { return _rotY; }
double Mechanics::Sensor::getRotZ() const { return _rotZ; }
double Mechanics::Sensor::getSensitiveX() const { return _sensitiveX; }
double Mechanics::Sensor::getSensitiveY() const { return _sensitiveY; }
const Mechanics::Device* Mechanics::Sensor::getDevice() const { return _device; }
const char* Mechanics::Sensor::getName() const { return _name.c_str(); }

