#ifndef CLUSTER_H
#define CLUSTER_H

#include <string>
#include <vector>

namespace Storage {

class Hit;
class Track;
class Plane;

class Cluster {
public:
  // Inline the setters and getters since they are used frequently
  void setPix(double x, double y)
  {
    m_pixX = x;
    m_pixY = y;
  }
  void setPos(double x, double y, double z)
  {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
  }
  void setPixErr(double x, double y)
  {
    m_pixErrX = x;
    m_pixErrY = y;
  }
  void setPosErr(double x, double y, double z)
  {
    m_posErrX = x;
    m_posErrY = y;
    m_posErrZ = z;
  }
  void setMatchDistance(double value) { m_matchDistance = value; }

  unsigned int getNumHits() const { return m_hits.size(); }
  double getPixX() const { return m_pixX; }
  double getPixY() const { return m_pixY; }
  double getPixErrX() const { return m_pixErrX; }
  double getPixErrY() const { return m_pixErrY; }
  double getPosX() const { return m_posX; }
  double getPosY() const { return m_posY; }
  double getPosZ() const { return m_posZ; }
  double getPosErrX() const { return m_posErrX; }
  double getPosErrY() const { return m_posErrY; }
  double getPosErrZ() const { return m_posErrZ; }
  double getTiming() const { return m_timing; }
  double getValue() const { return m_value; }
  double getMatchDistance() const { return m_matchDistance; }
  int getIndex() const { return m_index; }

  void setTrack(Track* track);
  void setMatchedTrack(Track* track);
  void addHit(Hit* hit); // Add a hit to this cluster

  // The implementation is in the cpp file so that the classes can be included
  Hit* getHit(unsigned int n) const { return m_hits.at(n); }
  Track* getTrack() const { return m_track; }
  Track* getMatchedTrack() const { return m_matchedTrack; }
  Plane* getPlane() const { return m_plane; }

  void print() const;
  const std::string printStr(int blankWidth = 0) const;

private:
  Cluster(); // The Event class manages the memory, not the user

  double m_pixX;
  double m_pixY;
  double m_pixErrX;
  double m_pixErrY;
  double m_posX; // Center of gravity's x position
  double m_posY;
  double m_posZ;
  double m_posErrX; // Uncertainty of the x measurement
  double m_posErrY;
  double m_posErrZ;
  double m_timing;        // The timing of the underlying hits
  double m_value;         // The total value of all its hits
  double m_matchDistance; // Distance to matched track

  int m_index;
  Plane* m_plane; //<! The plane containing the cluster
  Track* m_track;        // The track containing this cluster
  Track* m_matchedTrack; // Track matched to this cluster in DUT analysis (not
                        // stored)
  std::vector<Hit*> m_hits; // List of hits composing the cluster

  friend class Plane;     // Needs to use the set plane method
  friend class Event;     // Needs access the constructor and destructor
  friend class StorageIO; // Needs access to the track index
};

} // namespace Storage

#endif // CLUSTER_H
