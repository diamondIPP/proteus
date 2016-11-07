#ifndef PT_HIT_H
#define PT_HIT_H

#include <iosfwd>

#include "utils/definitions.h"

namespace Storage {

class Cluster;

/** A sensor hit identified by its address, timing, and value.
 *
 * To support devices where the recorded hit address does not directly
 * correspond to the pixel address in the physical pixel matrix, e.g. CCPDv4,
 * the Hit has separate digital (readout) and physical (pixel matrix)
 * addresses.
 */
class Hit {
public:
  /** Set address assuming identical digital and physical addresses. */
  void setAddress(Index col, Index row);
  /** Set only the physical address leaving the digital address untouched. */
  void setPhysicalAddress(Index col, Index row);
  void setValue(double value) { m_value = value; }
  void setTiming(double timing) { m_timing = timing; }
  /** Set global position using the transform from pixel to global coords. */
  void transformToGlobal(const Transform3D& pixelToGlobal);

  Index digitalCol() const { return m_digitalCol; }
  Index digitalRow() const { return m_digitalRow; }
  Index col() const { return m_col; }
  Index row() const { return m_row; }
  /** Return the pixel center position in pixel coordinates. */
  XYPoint posPixel() const { return XYPoint(col() + 0.5, row() + 0.5); }
  double timing() const { return m_timing; }
  double value() const { return m_value; }

  bool isInCluster() const { return (m_cluster != NULL); }
  const Cluster* cluster() const { return m_cluster; }
  void setCluster(const Cluster* cluster);

  unsigned int getPixX() const { return m_col; }
  unsigned int getPixY() const { return m_row; }
  double getPosX() const { return m_xyz.x(); }
  double getPosY() const { return m_xyz.y(); }
  double getPosZ() const { return m_xyz.z(); }
  double getValue() const { return m_value; }
  double getTiming() const { return m_timing; }

private:
  Hit(); // Hits memory is managed by the event class

  Index m_digitalCol, m_digitalRow;
  Index m_col, m_row;
  double m_timing; // Level 1 accept, typically
  double m_value;  // Time over threshold, typically
  XYZPoint m_xyz;
  const Cluster* m_cluster; // The cluster containing this hit

  friend class Plane;     // Access set plane method
  friend class Event;     // Access cluster index
  friend class StorageIO; // Constructor and destructor
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace Storage

inline void Storage::Hit::setAddress(Index col, Index row)
{
  m_digitalCol = m_col = col;
  m_digitalRow = m_row = row;
}

inline void Storage::Hit::setPhysicalAddress(Index col, Index row)
{
  m_col = col;
  m_row = row;
}

#endif // PT_HIT_H
