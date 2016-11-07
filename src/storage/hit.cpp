#include "hit.h"

#include <cassert>
#include <ostream>

#include "cluster.h"

Storage::Hit::Hit()
    : m_digitalCol(-1)
    , m_digitalRow(-1)
    , m_col(-1)
    , m_row(-1)
    , m_timing(-1)
    , m_value(-1)
    , m_cluster(NULL)
{
}

void Storage::Hit::transformToGlobal(const Transform3D& pixelToGlobal)
{
  // conversion from digital address to pixel center
  m_xyz = pixelToGlobal * XYZPoint(m_col + 0.5, m_row + 0.5, 0);
}

void Storage::Hit::setCluster(const Storage::Cluster* cluster)
{
  assert(!m_cluster && "Hit: can't cluster an already clustered hit.");
  m_cluster = cluster;
}

std::ostream& Storage::operator<<(std::ostream& os, const Storage::Hit& hit)
{
  if ((hit.digitalCol() != hit.col()) || (hit.digitalRow() != hit.row())) {
    os << "digital=(" << hit.digitalCol() << ", " << hit.digitalRow() << ") ";
  }
  os << "pixel=(" << hit.col() << ", " << hit.row() << ") ";
  os << "timing=" << hit.timing() << " value=" << hit.value();
  return os;
}
