#include "hit.h"

#include <cassert>
#include <ostream>

#include "cluster.h"

Storage::Hit::Hit()
    : m_digitalCol(-1)
    , m_digitalRow(-1)
    , m_col(-1)
    , m_row(-1)
    , m_time(-1)
    , m_value(-1)
    , m_cluster(NULL)
{
}

Storage::Hit::Hit(Index col, Index row, double time, double value)
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

void Storage::Hit::setPhysicalAddress(Index col, Index row)
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
