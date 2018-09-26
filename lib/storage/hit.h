#ifndef PT_HIT_H
#define PT_HIT_H

#include <iosfwd>

#include "utils/definitions.h"
#include "utils/interval.h"

namespace Storage {

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

  Hit(int col, int row, float time, float value);

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
  /** Return the pixel center position in pixel coordinates. */
  Vector2 posPixel() const { return {col() + 0.5, row() + 0.5}; }
  float time() const { return m_time; }
  float value() const { return m_value; }

  /** The area of the hit in pixel coordinates. */
  Area areaPixel() const;

  bool hasRegion() const { return m_region != kInvalidIndex; }
  Index region() const { return m_region; }

  bool isInCluster() const { return m_cluster != kInvalidIndex; }
  Index cluster() const { return m_cluster; }

private:
  int m_digitalCol, m_digitalRow;
  int m_col, m_row;
  float m_time;  // Level 1 accept, typically
  float m_value; // Time over threshold, typically
  Index m_region;
  Index m_cluster;
};

std::ostream& operator<<(std::ostream& os, const Hit& hit);

} // namespace Storage

#endif // PT_HIT_H
