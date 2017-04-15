#ifndef PT_HIT_H
#define PT_HIT_H

#include <iosfwd>

#include "utils/definitions.h"
#include "utils/interval.h"

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
  using Area = Utils::Box<2, int>;

  /** Set only the physical address leaving the digital address untouched. */
  void setPhysicalAddress(Index col, Index row);
  /** Set the region id. */
  void setRegion(Index region) { m_region = region; }

  Index region() const { return m_region; }
  Index digitalCol() const { return m_digitalCol; }
  Index digitalRow() const { return m_digitalRow; }
  Index col() const { return m_col; }
  Index row() const { return m_row; }
  /** Return the pixel center position in pixel coordinates. */
  XYPoint posPixel() const { return XYPoint(col() + 0.5, row() + 0.5); }
  double time() const { return m_time; }
  double value() const { return m_value; }

  /** The area of the hit in pixel coordinates. */
  Area areaPixel() const;

  void setCluster(const Cluster* cluster);
  const Cluster* cluster() const { return m_cluster; }

private:
  Hit(); // Hits memory is managed by the event class
  Hit(Index col, Index row, double time, double value);

  Index m_digitalCol, m_digitalRow;
  Index m_col, m_row;
  Index m_region;
  double m_time;            // Level 1 accept, typically
  double m_value;           // Time over threshold, typically
  const Cluster* m_cluster; // The cluster containing this hit

  friend class Event;       // Access cluster index
  friend class SensorEvent; // Access set plane method
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace Storage

inline Storage::Hit::Hit(Index col, Index row, double time, double value)
    : m_digitalCol(col)
    , m_digitalRow(row)
    , m_col(col)
    , m_row(row)
    , m_region(kInvalidIndex)
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

inline Storage::Hit::Area Storage::Hit::areaPixel() const
{
  return Area(Area::AxisInterval(m_col, m_col + 1),
              Area::AxisInterval(m_row, m_row + 1));
}

#endif // PT_HIT_H
