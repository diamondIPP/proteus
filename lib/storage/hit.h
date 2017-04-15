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

  Hit();
  Hit(Index col, Index row, double time, double value);

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
  Index m_digitalCol, m_digitalRow;
  Index m_col, m_row;
  Index m_region;
  double m_time;            // Level 1 accept, typically
  double m_value;           // Time over threshold, typically
  const Cluster* m_cluster; // The cluster containing this hit
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace Storage

#endif // PT_HIT_H
