#ifndef PT_HIT_H
#define PT_HIT_H

#include <iosfwd>

#include "utils/definitions.h"

namespace proteus {

/** A sensor hit identified by its address, timestamp, and value.
 *
 * To support devices where the recorded hit address does not directly
 * correspond to the pixel address in the physical pixel matrix, e.g. CCPDv4,
 * the Hit has separate digital (readout) and physical (pixel matrix)
 * addresses.
 */
class Hit {
public:
  Hit(int col, int row, int timestamp, int value);

  /** Set only the physical address leaving the digital address untouched. */
  void setPhysicalAddress(int col, int row);
  /** Set the region id. */
  void setRegion(Index region) { m_region = region; }
  /** Set the cluster index. */
  void setCluster(Index cluster);

  int digitalCol() const { return m_digitalCol; }
  int digitalRow() const { return m_digitalRow; }
  int col() const { return m_col; }
  int row() const { return m_row; }
  int timestamp() const { return m_timestamp; }
  int value() const { return m_value; }

  bool hasRegion() const { return m_region != kInvalidIndex; }
  Index region() const { return m_region; }

  bool isInCluster() const { return m_cluster != kInvalidIndex; }
  Index cluster() const { return m_cluster; }

private:
  int m_digitalCol;
  int m_digitalRow;
  int m_col;
  int m_row;
  int m_timestamp; // Level 1 accept, typically
  int m_value;     // Time over threshold, typically
  Index m_region;
  Index m_cluster;
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace proteus

#endif // PT_HIT_H
