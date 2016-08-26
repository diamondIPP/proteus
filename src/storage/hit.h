#ifndef HIT_H
#define HIT_H

#include <iosfwd>
#include <string>

#include "utils/definitions.h"

namespace Storage {

class Cluster;

class Hit {
public:
  void setPix(Index col, Index row)
  {
    m_col = col;
    m_row = row;
  }
  void setPos(double x, double y, double z) { m_pos.SetCoordinates(x, y, z); }
  void setGlobal(const XYZPoint& pos) { m_pos = pos; }
  void setValue(double value) { m_value = value; }
  void setTiming(double timing) { m_timing = timing; }

  Index col() const { return m_col; }
  Index row() const { return m_row; }
  XYPoint posPixel() const { return XYPoint(m_col, m_row); }
  XYZPoint posGlobal() const { return m_pos; }
  double value() const { return m_value; }
  double timing() const { return m_timing; }

  bool isInCluster() const { return (m_cluster != NULL); }
  Cluster* getCluster() const { return m_cluster; }
  void setCluster(Cluster* cluster);

  unsigned int getPixX() const { return m_col; }
  unsigned int getPixY() const { return m_row; }
  double getPosX() const { return posGlobal().x(); }
  double getPosY() const { return posGlobal().y(); }
  double getPosZ() const { return posGlobal().z(); }
  double getValue() const { return m_value; }
  double getTiming() const { return m_timing; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Hit(); // Hits memory is managed by the event class

  Index m_col, m_row;
  double m_value;  // Time over threshold, typically
  double m_timing; // Level 1 accept, typically
  XYZPoint m_pos;

  Cluster* m_cluster; // The cluster containing this hit

  friend class Plane;     // Access set plane method
  friend class Event;     // Access cluster index
  friend class StorageIO; // Constructor and destructor
};

} // namespace Storage

#endif // HIT_H
