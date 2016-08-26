#ifndef HIT_H
#define HIT_H

#include <string>

namespace Storage {

class Cluster;
class Plane;

class Hit {
public:
  void setPix(unsigned int x, unsigned int y)
  {
    _pixX = x;
    _pixY = y;
  }
  void setPos(double x, double y, double z)
  {
    _posX = x;
    _posY = y;
    _posZ = z;
  }
  void setValue(double value) { _value = value; }
  void setTiming(double timing) { _timing = timing; }

  unsigned int getPixX() const { return _pixX; }
  unsigned int getPixY() const { return _pixY; }
  double getPosX() const { return _posX; }
  double getPosY() const { return _posY; }
  double getPosZ() const { return _posZ; }
  double getValue() const { return _value; }
  double getTiming() const { return _timing; }

  void setCluster(Cluster* cluster);

  // These are in the cpp file so that the classes can be included
  Cluster* getCluster() const;
  Plane* getPlane() const;

  void print() const;
  const std::string printStr(int blankWidth = 0) const;

private:
  Hit(); // Hits memory is managed by the event class

  unsigned int _pixX; //<! pixel number (local x) of this hit
  unsigned int _pixY; //<! pixel number (local y) of this hit
  double _posX;       //<! X (global) position of the hit on the sensor
  double _posY;       //<! Y (global) position of the hit on the sensor
  double _posZ;       //<! Z (global) position of the hit on the sensor
  double _value;      // Time over threshold, typically
  double _timing;     // Level 1 accept, typically

  Cluster* _cluster; // The cluster containing this hit
  Plane* _plane; // Plane in which the hit is found

  friend class Plane;     // Access set plane method
  friend class Event;     // Access cluster index
  friend class StorageIO; // Constructor and destructor
};

} // namespace Storage

#endif // HIT_H
