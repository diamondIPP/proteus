#include "hit.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include "cluster.h"
#include "plane.h"

using std::cout;
using std::endl;

Storage::Hit::Hit() :
  _plane(NULL),
  _pixX(0),
  _pixY(0),
  _posX(0),
  _posY(0),
  _posZ(0),
  _value(0),
  _timing(0),
  _cluster(NULL)
{ }

void Storage::Hit::setCluster(Storage::Cluster *cluster){
  assert(!_cluster && "Hit: can't cluster an already clustered hit.");
  _cluster = cluster;
}

Storage::Cluster* Storage::Hit::getCluster() const { 
  return _cluster;
}

Storage::Plane* Storage::Hit::getPlane() const {
  return _plane;
}

void Storage::Hit::print() const {
  cout << "\nHIT\n" << printStr() << endl;
}

const std::string Storage::Hit::printStr(int blankWidth) const {
  std::ostringstream out;

  std::string blank(" ");
  for(int i=1; i<=blankWidth; i++) blank += " ";
  
  out << blank << "  Pix: (" << getPixX() << " , " << getPixY() << ")\n"
      << blank << "  Pos: (" << getPosX() << " , " << getPosY() << " , " << getPosZ() << ")\n"
      << blank << "  Value: " << getValue() << "\n"
      << blank << "  Timing: " << getTiming() << "\n"
      << blank << "  Cluster: " << getCluster() << "\n"
      << blank << "  Plane: "  << getPlane();

  return out.str();
}
