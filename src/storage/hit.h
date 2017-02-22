#ifndef PT_HIT_H
#define PT_HIT_H

#include <iosfwd>

#include "utils/definitions.h"

namespace Storage {

class Cluster;

/** A sensor hit identified by its address, time, and value.
 *
 * To support devices where the recorded hit address does not directly
 * correspond to the pixel address in the physical pixel matrix, e.g. CCPDv4,
 * the Hit has separate digital (readout) and physical (pixel matrix)
 * addresses.
 */
class Hit {
public:
  /** Set only the physical address leaving the digital address untouched. */
  void setPhysicalAddress(Index col, Index row);

  Index digitalCol() const { return m_digitalCol; }
  Index digitalRow() const { return m_digitalRow; }
  Index col() const { return m_col; }
  Index row() const { return m_row; }
  /** Return the pixel center position in pixel coordinates. */
  XYPoint posPixel() const { return XYPoint(col() + 0.5, row() + 0.5); }
  double time() const { return m_time; }
  double value() const { return m_value; }

  void setCluster(const Cluster* cluster);
  const Cluster* cluster() const { return m_cluster; }

  unsigned int getPixX() const { return m_col; }
  unsigned int getPixY() const { return m_row; }
  /** \deprecated Only clusters have global position information. */
  double getPosX() const { return NAN; }
  double getPosY() const { return NAN; }
  double getPosZ() const { return NAN; }
  double getValue() const { return value(); }
  double getTiming() const { return time(); }

private:
  Hit(); // Hits memory is managed by the event class
  Hit(Index col, Index row, double time, double value);

  Index m_digitalCol, m_digitalRow;
  Index m_col, m_row;
  double m_time;            // Level 1 accept, typically
  double m_value;           // Time over threshold, typically
  const Cluster* m_cluster; // The cluster containing this hit

  friend class Plane;     // Access set plane method
  friend class Event;     // Access cluster index
  friend class StorageIO; // Constructor and destructor
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace Storage

inline Storage::Hit::Hit(Index col, Index row, double time, double value)
    : m_digitalCol(col)
    , m_digitalRow(row)
    , m_col(col)
    , m_row(row)
    , m_time(time)
    , m_value(value)
    , m_cluster(nullptr)
{
}

inline void Storage::Hit::setPhysicalAddress(Index col, Index row)
{
  m_col = col;
  m_row = row;
}

#endif // PT_HIT_H
