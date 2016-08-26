#include "hit.h"

#include <cassert>
#include <ostream>

#include "cluster.h"
#include "plane.h"

Storage::Hit::Hit()
    : m_col(-1)
    , m_row(-1)
    , m_value(0)
    , m_timing(0)
    , m_cluster(NULL)
    , m_plane(NULL)
{
}

void Storage::Hit::setCluster(Storage::Cluster* cluster)
{
  assert(!m_cluster && "Hit: can't cluster an already clustered hit.");
  m_cluster = cluster;
}

Storage::Cluster* Storage::Hit::getCluster() const { return m_cluster; }

Storage::Plane* Storage::Hit::getPlane() const { return m_plane; }

void Storage::Hit::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "pixel: " << pixel() << '\n';
  os << prefix << "global: " << global() << '\n';
  os << prefix << "value: " << value() << '\n';
  os << prefix << "timing: " << timing() << '\n';
  os.flush();
}
