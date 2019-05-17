/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#include "densemask.h"

#include <iomanip>
#include <limits>
#include <ostream>

namespace proteus {

DenseMask::DenseMask() : DenseMask(0, 0, 0, 0) {}

// create an empty mask, i.e. no masked pixels, for the given area
DenseMask::DenseMask(int col0, int row0, int sizeCol, int sizeRow)
    : m_col0(col0)
    , m_col1(col0 + sizeCol)
    , m_row0(row0)
    , m_row1(row0 + sizeRow)
    , m_mask(sizeCol * sizeRow, false)
{
  assert(0 <= sizeCol);
  assert(0 <= sizeRow);
}

DenseMask::DenseMask(const std::set<ColumnRow>& masked)
    : m_col0(std::numeric_limits<int>::max())
    , m_col1(std::numeric_limits<int>::min())
    , m_row0(std::numeric_limits<int>::max())
    , m_row1(std::numeric_limits<int>::min())
{
  // first iteration to get boundaries
  for (const auto& pos : masked) {
    m_col0 = std::min<int>(m_col0, std::get<0>(pos));
    m_col1 = std::max<int>(m_col1, std::get<0>(pos) + 1);
    m_row0 = std::min<int>(m_row0, std::get<1>(pos));
    m_row1 = std::max<int>(m_row1, std::get<1>(pos) + 1);
  }
  // second iteration to fill the mask
  m_mask.resize((m_col1 - m_col0) * (m_row1 - m_row0), false);
  for (const auto& pos : masked) {
    m_mask[index(std::get<0>(pos), std::get<1>(pos))] = true;
  }
}

DenseMask DenseMask::protruded(int offset) const
{
  assert(0 <= offset);

  int newSizeCol = (m_col1 - m_col0) + 2 * offset;
  int newSizeRow = (m_row1 - m_row0) + 2 * offset;
  DenseMask bigger(m_col0 - offset, m_row0 - offset, newSizeCol, newSizeRow);

  for (int c = m_col0; c < m_col1; ++c) {
    for (int r = m_row0; r < m_row1; ++r) {
      if (isMasked(c, r)) {
        // set rectangular area around the masked pixel w/ the given offset
        for (int mc = c - offset; mc <= (c + offset); ++mc) {
          for (int mr = r - offset; mr <= (r + offset); ++mr) {
            bigger.m_mask[bigger.index(mc, mr)] = true;
          }
        }
      }
    }
  }
  return bigger;
}

std::ostream& operator<<(std::ostream& os, const DenseMask& pm)
{
  // print header line w/ column numbers
  os << "     ";
  for (int c = pm.m_col0; c < pm.m_col1; c += 5)
    os << std::setw(4) << std::left << c << ' ';
  os << "\n    +";
  for (int c = pm.m_col0; c < pm.m_col1; ++c)
    os << '-';
  os << "+\n";
  // print each row starting w/ the row number
  for (int r = (pm.m_row1 - 1); pm.m_row0 <= r; --r) {
    os << std::setw(4) << std::right << r << '|';
    for (int c = pm.m_col0; c < pm.m_col1; ++c) {
      os << (pm.isMasked(c, r) ? 'X' : ' ');
    }
    os << "|\n";
  }
  // print bottom line w/ column numbers
  os << "    +";
  for (int c = pm.m_col0; c < pm.m_col1; ++c)
    os << '-';
  os << "+\n     ";
  for (int c = pm.m_col0; c < pm.m_col1; c += 5)
    os << std::setw(4) << std::left << c << ' ';
  return os;
}

} // namespace proteus
