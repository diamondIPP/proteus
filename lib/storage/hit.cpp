#include "hit.h"

#include <cassert>
#include <ostream>

#include "cluster.h"

Storage::Hit::Hit(int col, int row, float time, float value)
    : m_digitalCol(col)
    , m_digitalRow(row)
    , m_col(col)
    , m_row(row)
    , m_time(time)
    , m_value(value)
    , m_region(kInvalidIndex)
    , m_cluster(nullptr)
{
}

void Storage::Hit::setPhysicalAddress(int col, int row)
{
  m_col = col;
  m_row = row;
}

void Storage::Hit::setCluster(const Storage::Cluster* cluster)
{
  assert(!m_cluster && "Hit: can't cluster an already clustered hit.");
  m_cluster = cluster;
}

Storage::Hit::Area Storage::Hit::areaPixel() const
{
  return Area(Area::AxisInterval(m_col, m_col + 1),
              Area::AxisInterval(m_row, m_row + 1));
}

std::ostream& Storage::operator<<(std::ostream& os, const Storage::Hit& hit)
{
  if ((hit.digitalCol() != hit.col()) || (hit.digitalRow() != hit.row())) {
    os << "digital=(" << hit.digitalCol() << ", " << hit.digitalRow() << ") ";
  }
  os << "pixel=(" << hit.col() << ", " << hit.row() << ") ";
  os << "time=" << hit.time() << " value=" << hit.value();
  return os;
}
