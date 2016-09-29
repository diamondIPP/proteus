#include "hit.h"

#include <cassert>
#include <ostream>

#include "cluster.h"

Storage::Hit::Hit()
    : m_col(-1)
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

void Storage::Hit::setCluster(Storage::Cluster* cluster)
{
  assert(!m_cluster && "Hit: can't cluster an already clustered hit.");
  m_cluster = cluster;
}

std::ostream& operator<<(std::ostream& os, const Storage::Hit& hit)
{
  os << "col=" << hit.col() << ", row=" << hit.row()
     << ", timing=" << hit.timing() << ", value=" << hit.value();
  return os;
}
