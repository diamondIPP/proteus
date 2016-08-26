#ifndef CLUSTER_H
#define CLUSTER_H

#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Storage {

class Hit;
class Track;
class Plane;

class Cluster {
public:
  void setPix(double col, double row) { m_pixel.SetCoordinates(col, row); }
  void setPos(double x, double y, double z) { m_global.SetCoordinates(x, y, z); }
  void setPixErr(double col, double row) { m_pixelErr.SetCoordinates(col, row); }
  void setPosErr(double x, double y, double z)
  {
    m_globalErr.SetCoordinates(x, y, z);
  }
  void setMatchDistance(double value) { m_matchDistance = value; }

  const XYPoint& posPixel() const { return m_pixel; }
  const XYZPoint& posGlobal() const { return m_global; }
  const XYVector& errPixel() const { return m_pixelErr; }
  const XYZVector& errGlobal() const { return m_globalErr; }
  double timing() const { return m_timing; }
  double value() const { return m_value; }

  Index getNumHits() const { return m_hits.size(); }
  Hit* getHit(Index i) const { return m_hits.at(i); }

  double getPixX() const { return posPixel().x(); }
  double getPixY() const { return posPixel().y(); }
  double getPixErrX() const { return errPixel().x(); }
  double getPixErrY() const { return errPixel().y(); }
  double getPosX() const { return posGlobal().x(); }
  double getPosY() const { return posGlobal().y(); }
  double getPosZ() const { return posGlobal().z(); }
  double getPosErrX() const { return errGlobal().x(); }
  double getPosErrY() const { return errGlobal().y(); }
  double getPosErrZ() const { return errGlobal().z(); }
  double getTiming() const { return timing(); }
  double getValue() const { return value(); }
  double getMatchDistance() const { return m_matchDistance; }
  int getIndex() const { return m_index; }

  void setTrack(Track* track);
  void setMatchedTrack(Track* track);
  void addHit(Hit* hit); // Add a hit to this cluster

  Track* getTrack() const { return m_track; }
  Track* getMatchedTrack() const { return m_matchedTrack; }
  Plane* getPlane() const { return m_plane; }

  void print() const;
  const std::string printStr(int blankWidth = 0) const;

private:
  Cluster(); // The Event class manages the memory, not the user

  std::vector<Hit*> m_hits; // List of hits composing the cluster
  XYPoint m_pixel;
  XYVector m_pixelErr;
  double m_timing;        // The timing of the underlying hits
  double m_value;         // The total value of all its hits
  XYZPoint m_global;
  XYZVector m_globalErr;

  int m_index;
  Plane* m_plane; //<! The plane containing the cluster
  Track* m_track;        // The track containing this cluster
  Track* m_matchedTrack; // Track matched to this cluster in DUT analysis (not
                        // stored)
  double m_matchDistance; // Distance to matched track

  friend class Plane;     // Needs to use the set plane method
  friend class Event;     // Needs access the constructor and destructor
  friend class StorageIO; // Needs access to the track index
};

} // namespace Storage

#endif // CLUSTER_H
