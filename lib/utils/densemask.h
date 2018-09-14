/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#ifndef PT_DENSEMASK_H
#define PT_DENSEMASK_H

#include <cassert>
#include <iosfwd>
#include <set>
#include <vector>

#include "utils/definitions.h"

namespace Utils {

/** Dense pixel mask.
 *
 * Stores a bit mask for the masked pixels to allow fast lookup and provides
 * some mask manipulation, e.g. protuding the mask to mask the nearest
 * neighboring pixels.
 */
class DenseMask {
public:
  /** Construct an empty mask. */
  DenseMask();
  /** Construct from a list of masked pixels. */
  DenseMask(const std::set<ColumnRow>& masked);

  /** Check if the given pixel address is masked. */
  bool isMasked(int col, int row) const;
  /** Check if the position in pixel coordinates is inside a masked pixel. */
  bool isMasked(const Vector2& cr) const;

  /** Return a new mask where the masked area is outset by the given offset. */
  DenseMask protruded(int offset) const;

private:
  DenseMask(int col0, int row0, int sizeCol, int sizeRow);

  size_t index(int col, int row) const;

  int m_col0, m_col1;
  int m_row0, m_row1;
  std::vector<bool> m_mask;

  friend std::ostream& operator<<(std::ostream& os, const DenseMask& pm);
};

std::ostream& operator<<(std::ostream& os, const DenseMask& pm);

} // namespace Utils

// linear index into the boolean mask
inline size_t Utils::DenseMask::index(int col, int row) const
{
  assert((m_col0 <= col) && (col < m_col1));
  assert((m_row0 <= row) && (row < m_row1));
  return (m_row1 - m_row0) * (col - m_col0) + (row - m_row0);
}

inline bool Utils::DenseMask::isMasked(int col, int row) const
{
  bool isInsideMask =
      (m_col0 <= col) && (col < m_col1) && (m_row0 <= row) && (row < m_row1);
  return isInsideMask && m_mask[index(col, row)];
}

inline bool Utils::DenseMask::isMasked(const Vector2& cr) const
{
  // the pixel area has a range [0, 1) along each axis
  return isMasked(static_cast<int>(std::floor(cr[0])),
                  static_cast<int>(std::floor(cr[1])));
}

#endif // PT_DENSEMASK_H
