/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#include "densemask.h"

#include <limits>
#include <ostream>

// create an empty mask, i.e. no masked pixels, for the given area
Utils::DenseMask::DenseMask(int col0, int row0, int sizeCol, int sizeRow)
    : m_col0(col0)
    , m_row0(row0)
    , m_sizeCol(sizeCol)
    , m_sizeRow(sizeRow)
    , m_mask(sizeCol * sizeRow, false)
{
  assert(0 <= sizeCol);
  assert(0 <= sizeRow);
}

Utils::DenseMask::DenseMask() : DenseMask(0, 0, 0, 0) {}

Utils::DenseMask::DenseMask(const std::set<ColumnRow>& masked)
    : m_col0(std::numeric_limits<int>::max())
    , m_row0(std::numeric_limits<int>::max())
    , m_sizeCol(0)
    , m_sizeRow(0)
{
  // first iteration to get boundaries
  for (const auto& pos : masked) {
    m_col0 = std::min<int>(m_col0, std::get<0>(pos));
    m_row0 = std::min<int>(m_row0, std::get<1>(pos));
    m_sizeCol = std::max<int>(m_sizeCol, std::get<0>(pos) - m_col0 + 1);
    m_sizeRow = std::max<int>(m_sizeRow, std::get<1>(pos) - m_row0 + 1);
  }
  // second iteration to fill the mask
  m_mask.resize(m_sizeCol * m_sizeRow, false);
  for (const auto& pos : masked)
    m_mask[index(std::get<0>(pos), std::get<1>(pos))] = true;
}

Utils::DenseMask Utils::DenseMask::protruded(int offset) const
{
  assert(0 <= offset);

  DenseMask bigger(m_col0 - offset, m_row0 - offset, m_sizeCol + 2 * offset,
                   m_sizeRow + 2 * offset);

  for (int c = m_col0; c < (m_col0 + m_sizeCol); ++c) {
    for (int r = m_row0; r < (m_row0 + m_sizeRow); ++r) {
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

std::ostream& Utils::operator<<(std::ostream& os, const DenseMask& pm)
{
  // print header line w/ column numbers
  os << "     ";
  for (int c = pm.m_col0; c < (pm.m_col0 + pm.m_sizeCol); c += 5)
    os << std::setw(4) << std::left << c << ' ';
  os << "\n    +";
  for (int c = pm.m_col0; c < (pm.m_col0 + pm.m_sizeCol); ++c)
    os << '-';
  os << "+\n";
  // print each row starting w/ the row number
  for (int r = (pm.m_row0 + pm.m_sizeRow - 1); pm.m_row0 <= r; --r) {
    os << std::setw(4) << std::right << r << '|';
    for (int c = pm.m_col0; c < (pm.m_col0 + pm.m_sizeCol); ++c) {
      os << (pm.isMasked(c, r) ? 'X' : ' ');
    }
    os << "|\n";
  }
  // print bottom line w/ column numbers
  os << "    +";
  for (int c = pm.m_col0; c < (pm.m_col0 + pm.m_sizeCol); ++c)
    os << '-';
  os << "+\n     ";
  for (int c = pm.m_col0; c < (pm.m_col0 + pm.m_sizeCol); c += 5)
    os << std::setw(4) << std::left << c << ' ';
  return os;
}
