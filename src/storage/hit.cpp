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

void Storage::Hit::setCluster(Storage::Cluster* cluster)
{
  assert(!m_cluster && "Hit: can't cluster an already clustered hit.");
  m_cluster = cluster;
}

void Storage::Hit::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "pixel: " << posPixel() << '\n';
  os << prefix << "global: " << posGlobal() << '\n';
  os << prefix << "timing: " << timing() << '\n';
  os << prefix << "value: " << value() << '\n';
  os.flush();
}
