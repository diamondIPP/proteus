#ifndef PT_HIT_H
#define PT_HIT_H

#include <iosfwd>

#include "utils/definitions.h"

namespace Storage {

class Cluster;

class Hit {
public:
  void setAddress(Index col, Index row) { m_col = col, m_row = row; }
  void setValue(double value) { m_value = value; }
  void setTiming(double timing) { m_timing = timing; }
  /** Set global position using the transform from pixel to global coords. */
  void transformToGlobal(const Transform3D& pixelToGlobal);

  Index col() const { return m_col; }
  Index row() const { return m_row; }
  /** Return the pixel center position in pixel coordinates. */
  XYPoint posPixel() const { return XYPoint(m_col + 0.5, m_row + 0.5); }
  const XYZPoint& posGlobal() const { return m_xyz; }
  double timing() const { return m_timing; }
  double value() const { return m_value; }

  bool isInCluster() const { return (m_cluster != NULL); }
  Cluster* cluster() { return m_cluster; }
  const Cluster* cluster() const { return m_cluster; }
  void setCluster(Cluster* cluster);

  unsigned int getPixX() const { return m_col; }
  unsigned int getPixY() const { return m_row; }
  double getPosX() const { return posGlobal().x(); }
  double getPosY() const { return posGlobal().y(); }
  double getPosZ() const { return posGlobal().z(); }
  double getValue() const { return m_value; }
  double getTiming() const { return m_timing; }

private:
  Hit(); // Hits memory is managed by the event class

  Index m_col, m_row;
  double m_timing; // Level 1 accept, typically
  double m_value;  // Time over threshold, typically
  XYZPoint m_xyz;
  Cluster* m_cluster; // The cluster containing this hit

  friend class Plane;     // Access set plane method
  friend class Event;     // Access cluster index
  friend class StorageIO; // Constructor and destructor
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace Storage

#endif // PT_HIT_H
