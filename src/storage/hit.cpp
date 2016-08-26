#include "hit.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include "cluster.h"
#include "plane.h"

using std::cout;
using std::endl;

Storage::Hit::Hit()
    :  m_col(-1)
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

void Storage::Hit::print() const { cout << "\nHIT\n" << printStr() << endl; }

const std::string Storage::Hit::printStr(int blankWidth) const
{
  std::ostringstream out;

  std::string blank(" ");
  for (int i = 1; i <= blankWidth; i++)
    blank += " ";

  out << blank << "  Pix: (" << getPixX() << " , " << getPixY() << ")\n"
      << blank << "  Pos: (" << getPosX() << " , " << getPosY() << " , "
      << getPosZ() << ")\n"
      << blank << "  Value: " << getValue() << "\n"
      << blank << "  Timing: " << getTiming() << "\n"
      << blank << "  Cluster: " << getCluster() << "\n"
      << blank << "  Plane: " << getPlane();

  return out.str();
}
