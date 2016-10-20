#ifndef CLUSTER_H
#define CLUSTER_H

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Storage {

class Hit;
class Track;
class Plane;

class Cluster {
public:
  void setPosPixel(const XYPoint& cr) { m_cr = cr; }
  void setErrPixel(const XYVector& err) { m_errCr = err; }
  /** Set global position using the transform from pixel to global coords. */
  void transformToGlobal(const Transform3D& pixelToGlobal);
  void setTiming(double timing) { m_timing = timing; }

  const XYPoint& posPixel() const { return m_cr; }
  const XYZPoint& posGlobal() const { return m_xyz; }
  const XYVector& errPixel() const { return m_errCr; }
  const XYZVector& errGlobal() const { return m_errXyz; }
  double timing() const { return m_timing; }
  double value() const { return m_value; }

  Index sensorId() const;

  void addHit(Hit* hit);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return m_hits.at(i); }
  const Hit* getHit(Index i) const { return m_hits.at(i); }

  const Track* track() const { return m_track; }

  Index getNumHits() const { return m_hits.size(); }
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
  void setMatchedTrack(Track* track) { m_matchedTrack = track; }

  Track* getTrack() const { return m_track; }
  Track* getMatchedTrack() const { return m_matchedTrack; }
  Plane* getPlane() const { return m_plane; }

  void setMatchDistance(double value) { m_matchDistance = value; }
  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Cluster(); // The Event class manages the memory, not the user

  XYPoint m_cr;
  XYVector m_errCr;
  double m_timing; // The timing of the underlying hits
  double m_value;  // The combined value of all its hits
  XYZPoint m_xyz;
  XYZVector m_errXyz;
  std::vector<Hit*> m_hits; // List of hits composing the cluster

  int m_index;
  Plane* m_plane;         //<! The plane containing the cluster
  Track* m_track;         // The track containing this cluster
  Track* m_matchedTrack;  // Track matched to this cluster in DUT analysis (not
                          // stored)
  double m_matchDistance; // Distance to matched track

  friend class Plane;     // Needs to use the set plane method
  friend class Event;     // Needs access the constructor and destructor
  friend class StorageIO; // Needs access to the track index
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // CLUSTER_H
