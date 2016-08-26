#ifndef HIT_H
#define HIT_H

#include <string>

namespace Storage {

class Cluster;
class Plane;

class Hit {
protected:
  Hit(); // Hits memory is managed by the event class

public:
  friend class Plane;     // Access set plane method
  friend class Event;     // Access cluster index
  friend class StorageIO; // Constructor and destructor

  // Inline setters and getters since they will be used frequently
  inline void setPix(unsigned int x, unsigned int y)
  {
    _pixX = x;
    _pixY = y;
  }
  inline void setPos(double x, double y, double z)
  {
    _posX = x;
    _posY = y;
    _posZ = z;
  }
  inline void setValue(double value) { _value = value; }
  inline void setTiming(double timing) { _timing = timing; }

  inline unsigned int getPixX() const { return _pixX; }
  inline unsigned int getPixY() const { return _pixY; }
  inline double getPosX() const { return _posX; }
  inline double getPosY() const { return _posY; }
  inline double getPosZ() const { return _posZ; }
  inline double getValue() const { return _value; }
  inline double getTiming() const { return _timing; }

  void setCluster(Cluster* cluster);

  // These are in the cpp file so that the classes can be included
  Cluster* getCluster() const;
  Plane* getPlane() const;

  void print() const;
  const std::string printStr(int blankWidth = 0) const;

protected:
  Plane* _plane; // Plane in which the hit is found

private:
  unsigned int _pixX; //<! pixel number (local x) of this hit
  unsigned int _pixY; //<! pixel number (local y) of this hit
  double _posX;       //<! X (global) position of the hit on the sensor
  double _posY;       //<! Y (global) position of the hit on the sensor
  double _posZ;       //<! Z (global) position of the hit on the sensor
  double _value;      // Time over threshold, typically
  double _timing;     // Level 1 accept, typically

  Cluster* _cluster; // The cluster containing this hit

}; // end of class

} // end of namespace

#endif // HIT_H
